#include <QtTest>
#include "rdp/RdpInput.h"
#include "rdp/RdpSettings.h"
#include "app/ConnectionHistory.h"
#include "profiles/ConnectionProfile.h"
#include "profiles/ProfileStore.h"
#include "profiles/RdpFile.h"
#include "profiles/RdpProfileMapper.h"
#include "display/MonitorTopology.h"
#include "channels/ClipboardText.h"
#include "channels/AudioConfiguration.h"
#include "channels/DriveRedirection.h"
#include "channels/PrinterRedirection.h"
#include <QTemporaryDir>
#include <freerdp/scancode.h>
#include <winpr/input.h>

class TestCore final : public QObject {
    Q_OBJECT
private slots:
    void servers() {
        const auto plain = openrdp::parseServer(QStringLiteral("server01"));
        QVERIFY(plain); QCOMPARE(plain->hostname, QStringLiteral("server01")); QCOMPARE(plain->port, 3389);
        const auto custom = openrdp::parseServer(QStringLiteral("10.10.1.10:3390"));
        QVERIFY(custom); QCOMPARE(custom->port, 3390);
        const auto ipv6 = openrdp::parseServer(QStringLiteral("[2001:db8::1]:3390"));
        QVERIFY(ipv6); QCOMPARE(ipv6->hostname, QStringLiteral("2001:db8::1"));
        QVERIFY(!openrdp::parseServer(QStringLiteral("server:0")));
        QVERIFY(!openrdp::parseServer(QStringLiteral("server:65536")));
        QVERIFY(!openrdp::parseServer(QStringLiteral("server:abc")));
    }
    void usernames() {
        QString user, domain;
        openrdp::parseUsername(QStringLiteral("CONTOSO\\jsmith"), user, domain);
        QCOMPARE(user, QStringLiteral("jsmith")); QCOMPARE(domain, QStringLiteral("CONTOSO"));
        openrdp::parseUsername(QStringLiteral("jsmith@contoso.com"), user, domain);
        QCOMPARE(user, QStringLiteral("jsmith@contoso.com")); QVERIFY(domain.isEmpty());
    }
    void scaling() {
        QCOMPARE(openrdp::scaleMousePosition({640, 360}, {1280, 720}, {1920, 1080}), QPoint(960, 540));
    }
    void displaySizeConstraints() {
        QCOMPARE(openrdp::constrainedDisplaySize({100, 199}), QSize(200, 200));
        QCOMPARE(openrdp::constrainedDisplaySize({1920, 1080}), QSize(1920, 1080));
        QCOMPARE(openrdp::constrainedDisplaySize({9000, 12000}), QSize(8192, 8192));
    }
    void monitorTopologyConversion() {
        const QVector<openrdp::MonitorInfo> monitors{
            {QStringLiteral("portrait"), QRect(-1080, -300, 1080, 1920), QSize(300, 530), 1.25, 90, false},
            {QStringLiteral("primary"), QRect(0, 0, 2560, 1440), QSize(600, 340), 1.5, 0, true},
            {QStringLiteral("right"), QRect(2560, 200, 1920, 1080), QSize(510, 290), 1.0, 0, false}};
        QString error; QVERIFY2(openrdp::validMonitorTopology(monitors, &error), qPrintable(error));
        const auto converted = openrdp::toFreeRdpMonitors(monitors);
        QCOMPARE(converted.size(), 3);
        QCOMPARE(converted.at(0).x, -1080); QCOMPARE(converted.at(0).y, -300);
        QCOMPARE(converted.at(0).attributes.orientation, 90U);
        QCOMPARE(converted.at(1).is_primary, 1U);
        QCOMPARE(converted.at(1).attributes.desktopScaleFactor, 150U);
        QCOMPARE(converted.at(2).x, 2560); QCOMPARE(converted.at(2).width, 1920);
    }
    void monitorTopologyValidation() {
        QString error;
        QVERIFY(!openrdp::validMonitorTopology({}, &error));
        QVector<openrdp::MonitorInfo> duplicate{
            {QStringLiteral("same"), QRect(0,0,1920,1080), QSize(500,300), 1.0, 0, true},
            {QStringLiteral("same"), QRect(1920,0,1920,1080), QSize(500,300), 1.0, 0, false}};
        QVERIFY(!openrdp::validMonitorTopology(duplicate, &error));
        duplicate[1].id=QStringLiteral("second"); duplicate[1].primary=true;
        QVERIFY(!openrdp::validMonitorTopology(duplicate, &error));
    }
    void normalizedMonitorPresentationRects() {
        const QVector<openrdp::MonitorInfo> monitors{
            {QStringLiteral("left"),QRect(-1920,0,1920,1080),QSize(500,300),1.0,0,false},
            {QStringLiteral("primary"),QRect(0,0,2560,1440),QSize(600,340),1.0,0,true},
            {QStringLiteral("upper"),QRect(2560,-900,1600,900),QSize(400,230),1.0,0,false}};
        const auto rects=openrdp::normalizedMonitorRects(monitors);
        QCOMPARE(rects,QVector<QRect>({QRect(0,900,1920,1080),QRect(1920,900,2560,1440),QRect(4480,0,1600,900)}));
    }
    void clipboardUnicodeRoundTrip() {
        const QString source=QString::fromUtf8("Hello\nGrüße — 世界 🚀");
        const QByteArray encoded=openrdp::encodeClipboardText(source);
        QVERIFY(encoded.endsWith(QByteArray(2,'\0')));QVERIFY(encoded.size()%2==0);
        const auto decoded=openrdp::decodeClipboardText(encoded);QVERIFY(decoded);QCOMPARE(*decoded,source);
        QVERIFY(!openrdp::decodeClipboardText(QByteArrayLiteral("odd")));
        QVERIFY(!openrdp::decodeClipboardText(QByteArray(16*1024*1024+2,'x')));
    }
    void audioChannelArguments() {
        openrdp::AudioConfiguration config;
        QCOMPARE(openrdp::rdpsndArguments(config),QVector<QByteArray>({"rdpsnd","sys:pulse"}));
        QVERIFY(openrdp::audinArguments(config).isEmpty());
        config.outputDevice=QStringLiteral("office sink");config.microphone=true;config.inputDevice=QStringLiteral("usb mic");
        QCOMPARE(openrdp::rdpsndArguments(config).last(),QByteArrayLiteral("dev:office sink"));
        QCOMPARE(openrdp::audinArguments(config),QVector<QByteArray>({"audin","sys:pulse","dev:usb mic"}));
        config.output=openrdp::RemoteAudioMode::Remote;QVERIFY(openrdp::rdpsndArguments(config).isEmpty());
    }
    void driveFolderValidation() {
        QTemporaryDir directory;QVERIFY(directory.isValid());QString error;
        const auto folder=openrdp::validateRedirectedFolder(QStringLiteral("Scripts"),directory.path(),&error);QVERIFY2(folder,qPrintable(error));
        QCOMPARE(folder->canonicalPath,QFileInfo(directory.path()).canonicalFilePath());
        QCOMPARE(openrdp::driveArguments(*folder),QVector<QByteArray>({"drive","Scripts",folder->canonicalPath.toUtf8()}));
        QVERIFY(!openrdp::validateRedirectedFolder(QStringLiteral("bad/name"),directory.path(),&error));
        QVERIFY(!openrdp::validateRedirectedFolder(QStringLiteral("Root"),QStringLiteral("/"),&error));
        QVERIFY(!openrdp::validateRedirectedFolder(QStringLiteral("Missing"),directory.filePath(QStringLiteral("missing")),&error));
        const QString linkPath=directory.filePath(QStringLiteral("link"));
        if(QFile::link(directory.path(),linkPath))QVERIFY(!openrdp::validateRedirectedFolder(QStringLiteral("Link"),linkPath,&error));
        QVector<openrdp::RedirectedFolderConfig> duplicate{*folder,{QStringLiteral("scripts"),folder->canonicalPath}};
        QVERIFY(!openrdp::uniqueFolderNames(duplicate,&error));
    }
    void printerRedirectionArguments() {
        QString error;const auto printer=openrdp::validatePrinter({QStringLiteral("Office_LaserJet"),QStringLiteral("Office LaserJet"),true},&error);QVERIFY2(printer,qPrintable(error));
        QCOMPARE(openrdp::printerArguments(*printer),QVector<QByteArray>({"printer","Office_LaserJet","MS Publisher Imagesetter","default"}));
        auto nonDefault=*printer;nonDefault.isDefault=false;
        QCOMPARE(openrdp::printerArguments(nonDefault),QVector<QByteArray>({"printer","Office_LaserJet"}));
        QVERIFY(!openrdp::validatePrinter({QStringLiteral("bad,name"),{},false},&error));
        QVector<openrdp::PrinterInfo> duplicate{*printer,{QStringLiteral("office_laserjet"),{},false}};QVERIFY(!openrdp::uniquePrinters(duplicate,&error));
        const auto discovered=openrdp::discoverCupsPrinters();for(const auto& item:discovered)QVERIFY2(openrdp::validatePrinter(item,&error),qPrintable(error));
    }
    void usKeyboardSymbols() {
        QCOMPARE(openrdp::qtKeyToVirtualKey(Qt::Key_Exclam), openrdp::qtKeyToVirtualKey(Qt::Key_1));
        QCOMPARE(openrdp::qtKeyToVirtualKey(Qt::Key_At), openrdp::qtKeyToVirtualKey(Qt::Key_2));
        QCOMPARE(openrdp::qtKeyToVirtualKey(Qt::Key_Underscore), openrdp::qtKeyToVirtualKey(Qt::Key_Minus));
        QCOMPARE(openrdp::qtKeyToVirtualKey(Qt::Key_Plus), openrdp::qtKeyToVirtualKey(Qt::Key_Equal));
        QVERIFY(openrdp::qtKeyToVirtualKey(Qt::Key_BraceLeft) != 0);
        QVERIFY(openrdp::qtKeyToVirtualKey(Qt::Key_Bar) != 0);
        QVERIFY(openrdp::qtKeyToVirtualKey(Qt::Key_Colon) != 0);
        QVERIFY(openrdp::qtKeyToVirtualKey(Qt::Key_QuoteDbl) != 0);
        QVERIFY(openrdp::qtKeyToVirtualKey(Qt::Key_Less) != 0);
        QVERIFY(openrdp::qtKeyToVirtualKey(Qt::Key_Question) != 0);
        QVERIFY(openrdp::qtKeyToVirtualKey(Qt::Key_AsciiTilde) != 0);
    }
    void modifierScancodes() {
        const auto shift = openrdp::qtKeyToVirtualKey(Qt::Key_Shift);
        const auto control = openrdp::qtKeyToVirtualKey(Qt::Key_Control);
        const auto alt = openrdp::qtKeyToVirtualKey(Qt::Key_Alt);
        QCOMPARE(shift, static_cast<std::uint32_t>(VK_LSHIFT));
        QCOMPARE(control, static_cast<std::uint32_t>(VK_LCONTROL));
        QCOMPARE(alt, static_cast<std::uint32_t>(VK_LMENU));
        QVERIFY(GetVirtualScanCodeFromVirtualKeyCode(shift, 4) != 0);
        QVERIFY(GetVirtualScanCodeFromVirtualKeyCode(control, 4) != 0);
        QVERIFY(GetVirtualScanCodeFromVirtualKeyCode(alt, 4) != 0);
    }
    void navigationScancodesAreExtended() {
        const std::array keys{VK_LEFT,VK_UP,VK_RIGHT,VK_DOWN,VK_PRIOR,VK_NEXT,VK_END,VK_HOME,VK_INSERT,VK_DELETE};
        for (const auto key : keys) QVERIFY(RDP_SCANCODE_EXTENDED(openrdp::virtualKeyToRdpScancode(key)));
        QCOMPARE(openrdp::virtualKeyToRdpScancode(VK_LEFT),static_cast<std::uint32_t>(RDP_SCANCODE_LEFT));
    }
    void connectionHistory() {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        openrdp::ConnectionHistory history(directory.filePath(QStringLiteral("history.json")));
        const openrdp::ConnectionHistoryEntry first{QStringLiteral("server01"),QStringLiteral("CONTOSO\\user"),openrdp::AuthenticationMode::NlaPassword};
        const openrdp::ConnectionHistoryEntry second{QStringLiteral("cloud.example"),QStringLiteral("user@example.com"),openrdp::AuthenticationMode::EntraWebAccount};
        QVERIFY(history.record(first)); QVERIFY(history.record(second));
        auto entries=history.load(); QCOMPARE(entries.size(),2); QCOMPARE(entries.at(0),second); QCOMPARE(entries.at(1),first);
        QVERIFY(history.record(first)); entries=history.load(); QCOMPARE(entries.size(),2); QCOMPARE(entries.at(0),first);
        QFile file(history.filePath()); QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray contents=file.readAll(); QVERIFY(!contents.contains("password")); QVERIFY(!contents.contains("token"));
        QVERIFY(entries.at(0).lastConnectedAt.isValid());QVERIFY(history.remove(first));QCOMPARE(history.load().size(),1);QVERIFY(history.clear());QVERIFY(history.load().isEmpty());
    }
    void aadRedirectParsing() {
        const auto code = openrdp::authorizationCodeFromRedirect(
            QStringLiteral("https://login.microsoftonline.com/common/oauth2/nativeclient?code=abc%2B123&session_state=x"));
        QVERIFY(code); QCOMPARE(*code, QStringLiteral("abc+123"));
        QVERIFY(!openrdp::authorizationCodeFromRedirect(QStringLiteral("http://example.test/?code=secret")));
        QVERIFY(!openrdp::authorizationCodeFromRedirect(QStringLiteral("https://login.microsoftonline.com/")));
        QVERIFY(!openrdp::authorizationCodeFromRedirect(QStringLiteral("https://login.microsoftonline.com/?error=access_denied")));
        QVERIFY(!openrdp::authorizationCodeFromRedirect(QStringLiteral("https://example.test/common/oauth2/nativeclient?code=secret")));
    }
    void rdpFileRoundTripPreservesProperties() {
        const QByteArray source = QByteArrayLiteral(
            "full address:s:server01.example.test:3390\r\n"
            "desktopwidth:i:1920\r\n"
            "redirectclipboard:i:1\r\n"
            "vendor extension:x:opaque:value\r\n"
            "desktopwidth:i:2560\r\n");
        const openrdp::RdpFileParser parser;
        const auto parsed = parser.parse(source);
        QVERIFY2(parsed, qPrintable(parsed.error));
        QCOMPARE(parsed.file.properties.size(), 5);
        QCOMPARE(parsed.file.properties.at(3).type, QChar(u'x'));
        QCOMPARE(parsed.file.properties.at(3).value, QStringLiteral("opaque:value"));
        const auto width = parsed.file.lastProperty(QStringLiteral("DesktopWidth"));
        QVERIFY(width); QCOMPARE(width->value, QStringLiteral("2560"));

        const QByteArray written = openrdp::RdpFileWriter().write(parsed.file);
        QVERIFY(written.startsWith(QByteArray::fromHex("fffe")));
        const auto reparsed = parser.parse(written);
        QVERIFY2(reparsed, qPrintable(reparsed.error));
        QCOMPARE(reparsed.file.properties, parsed.file.properties);
    }
    void rdpFileRejectsMalformedInput() {
        const openrdp::RdpFileParser parser;
        auto parsed = parser.parse(QByteArrayLiteral("not a property\n"));
        QVERIFY(!parsed); QCOMPARE(parsed.errorLine, 1);
        parsed = parser.parse(QByteArrayLiteral("desktopwidth:i:not-a-number\n"));
        QVERIFY(!parsed); QCOMPARE(parsed.errorLine, 1);
        QByteArray withNull = QByteArrayLiteral("full address:s:server");
        withNull.append('\0');
        withNull.append(QByteArrayLiteral("hidden"));
        parsed = parser.parse(withNull);
        QVERIFY(!parsed);
        parsed = parser.parse(QByteArray(openrdp::RdpFileParser::maximumFileSize + 1, 'a'));
        QVERIFY(!parsed);
    }
    void profileDefaultsAreConservative() {
        const openrdp::ConnectionProfile profile;
        QCOMPARE(profile.version, openrdp::ConnectionProfile::currentSchemaVersion);
        QCOMPARE(profile.resources.clipboard, openrdp::ClipboardSharing::Bidirectional);
        QCOMPARE(profile.resources.audioPlayback, openrdp::AudioPlayback::Local);
        QVERIFY(!profile.resources.microphone);
        QVERIFY(!profile.resources.printers);
        QVERIFY(profile.resources.folders.isEmpty());
        QCOMPARE(profile.display.mode, openrdp::DisplayMode::SingleMonitor);
        QCOMPARE(profile.display.resizeBehavior, openrdp::ResizeBehavior::Dynamic);
    }
    void rdpImportMapsSettingsAndQuarantinesResources() {
        const QByteArray source = QByteArrayLiteral(
            "full address:s:server01.example.test:3390\n"
            "username:s:alice\n"
            "domain:s:CONTOSO\n"
            "screen mode id:i:2\n"
            "desktopwidth:i:2560\n"
            "desktopheight:i:1440\n"
            "use multimon:i:1\n"
            "selectedmonitors:s:2, 0,2,bad\n"
            "dynamic resolution:i:0\n"
            "smart sizing:i:1\n"
            "audiomode:i:2\n"
            "redirectclipboard:i:1\n"
            "audiocapturemode:i:1\n"
            "redirectprinters:i:1\n"
            "drivestoredirect:s:home,downloads\n"
            "password 51:b:secret-blob\n"
            "vendor feature:s:preserve-me\n");
        const auto parsed = openrdp::RdpFileParser().parse(source);
        QVERIFY(parsed);
        const auto imported = openrdp::RdpProfileMapper().importFile(parsed.file);
        QCOMPARE(imported.profile.server, QStringLiteral("server01.example.test:3390"));
        QCOMPARE(imported.profile.username, QStringLiteral("CONTOSO\\alice"));
        QVERIFY(imported.profile.display.fullScreen);
        QCOMPARE(imported.profile.display.fixedWidth, 2560U);
        QCOMPARE(imported.profile.display.fixedHeight, 1440U);
        QCOMPARE(imported.profile.display.mode, openrdp::DisplayMode::MultipleMonitors);
        QCOMPARE(imported.profile.display.selectedMonitors, QVector<int>({2, 0}));
        QCOMPARE(imported.profile.display.resizeBehavior, openrdp::ResizeBehavior::ScaleToFit);
        QCOMPARE(imported.profile.resources.audioPlayback, openrdp::AudioPlayback::Disabled);
        QCOMPARE(imported.profile.resources.clipboard, openrdp::ClipboardSharing::Disabled);
        QVERIFY(!imported.profile.resources.microphone);
        QVERIFY(!imported.profile.resources.printers);
        QVERIFY(imported.profile.resources.folders.isEmpty());
        QCOMPARE(imported.resourceRequests.size(), 4);
        QVERIFY(imported.discardedPasswordProperty);
        QVERIFY(!imported.source.lastProperty(QStringLiteral("password 51")));
        QVERIFY(imported.source.lastProperty(QStringLiteral("vendor feature")));
        QVERIFY(imported.unsupportedKeys.contains(QStringLiteral("vendor feature")));
    }
    void rdpExportPreservesUnknownAndDropsPasswords() {
        openrdp::ConnectionProfile profile;
        profile.server = QStringLiteral("new-server");
        profile.username = QStringLiteral("user@example.test");
        profile.display.fullScreen = true;
        profile.resources.clipboard = openrdp::ClipboardSharing::Bidirectional;
        profile.resources.microphone = true;
        openrdp::RdpFile preserved{{
            {QStringLiteral("vendor feature"), u's', QStringLiteral("opaque")},
            {QStringLiteral("password 51"), u'b', QStringLiteral("must-not-survive")},
            {QStringLiteral("full address"), u's', QStringLiteral("old-server")}}};
        const auto exported = openrdp::RdpProfileMapper().exportFile(profile, preserved);
        QVERIFY(exported.lastProperty(QStringLiteral("vendor feature")));
        QVERIFY(!exported.lastProperty(QStringLiteral("password 51")));
        QCOMPARE(exported.lastProperty(QStringLiteral("full address"))->value, QStringLiteral("new-server"));
        QCOMPARE(exported.lastProperty(QStringLiteral("screen mode id"))->value, QStringLiteral("2"));
        QCOMPARE(exported.lastProperty(QStringLiteral("redirectclipboard"))->value, QStringLiteral("1"));
        const QByteArray bytes = openrdp::RdpFileWriter().write(exported);
        QVERIFY(!bytes.contains("must-not-survive"));
    }
    void profileStoreRoundTripAndNoSecrets() {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        openrdp::ProfileStore store(directory.path());
        openrdp::ConnectionProfile profile;
        profile.id = QStringLiteral("production-dc");
        profile.name = QStringLiteral("Production DC");
        profile.server = QStringLiteral("dc01.example.test");
        profile.username = QStringLiteral("CONTOSO\\alice");
        profile.favorite = true;
        profile.display.fullScreen = true;
        profile.display.mode = openrdp::DisplayMode::MultipleMonitors;
        profile.display.selectedMonitors = {1, 3};
        profile.resources.microphone = true;
        profile.resources.audioOutputDevice = QStringLiteral("office-output");
        profile.resources.audioInputDevice = QStringLiteral("conference-mic");
        profile.resources.printers = true;
        profile.resources.printerNames = {QStringLiteral("Office_LaserJet"), QStringLiteral("PDF")};
        profile.resources.folders.append({true, QStringLiteral("Scripts"), QStringLiteral("/srv/scripts")});
        QString error;
        QVERIFY2(store.save(profile, &error), qPrintable(error));
        QCOMPARE(store.profileIds(), QStringList({QStringLiteral("production-dc")}));
        const auto loaded = store.load(profile.id, &error);
        QVERIFY2(loaded, qPrintable(error));
        QCOMPARE(loaded->name, profile.name);
        QCOMPARE(loaded->server, profile.server);
        QCOMPARE(loaded->username, profile.username);
        QVERIFY(loaded->favorite);
        QVERIFY(loaded->display.fullScreen);
        QCOMPARE(loaded->display.selectedMonitors, profile.display.selectedMonitors);
        QCOMPARE(loaded->resources.audioOutputDevice, profile.resources.audioOutputDevice);
        QCOMPARE(loaded->resources.audioInputDevice, profile.resources.audioInputDevice);
        QVERIFY(loaded->resources.printers);
        QCOMPARE(loaded->resources.printerNames, profile.resources.printerNames);
        QCOMPARE(loaded->resources.folders.size(), 1);
        QFile file(directory.filePath(QStringLiteral("production-dc.json")));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray saved = file.readAll();
        QVERIFY(!saved.contains("password"));
        QVERIFY(!saved.contains("token"));
        QVERIFY(store.remove(profile.id, &error));
        QVERIFY(store.profileIds().isEmpty());
    }
    void profileStoreMigratesLegacyAndRejectsTraversal() {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        QFile legacy(directory.filePath(QStringLiteral("legacy.json")));
        QVERIFY(legacy.open(QIODevice::WriteOnly));
        legacy.write(R"({"host":"old.example.test","user":"legacy-user","webAccount":true})");
        legacy.close();
        openrdp::ProfileStore store(directory.path());
        QString error;
        const auto migrated = store.load(QStringLiteral("legacy"), &error);
        QVERIFY2(migrated, qPrintable(error));
        QCOMPARE(migrated->version, openrdp::ConnectionProfile::currentSchemaVersion);
        QCOMPARE(migrated->server, QStringLiteral("old.example.test"));
        QCOMPARE(migrated->username, QStringLiteral("legacy-user"));
        QCOMPARE(migrated->authenticationMode, openrdp::AuthenticationMode::EntraWebAccount);
        QVERIFY(!store.load(QStringLiteral("../legacy"), &error));
        openrdp::ConnectionProfile invalid;
        invalid.id = QStringLiteral("../../escape");
        invalid.server = QStringLiteral("server");
        QVERIFY(!store.save(invalid, &error));
        QFile future(directory.filePath(QStringLiteral("future.json")));
        QVERIFY(future.open(QIODevice::WriteOnly));
        future.write(R"({"version":999,"server":"example"})");
        future.close();
        QVERIFY(!store.load(QStringLiteral("future"), &error));
    }
};
QTEST_MAIN(TestCore)
#include "TestCore.moc"
