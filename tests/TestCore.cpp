#include <QtTest>
#include "rdp/RdpInput.h"
#include "rdp/RdpSettings.h"
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
    void aadRedirectParsing() {
        const auto code = openrdp::authorizationCodeFromRedirect(
            QStringLiteral("https://login.microsoftonline.com/common/oauth2/nativeclient?code=abc%2B123&session_state=x"));
        QVERIFY(code); QCOMPARE(*code, QStringLiteral("abc+123"));
        QVERIFY(!openrdp::authorizationCodeFromRedirect(QStringLiteral("http://example.test/?code=secret")));
        QVERIFY(!openrdp::authorizationCodeFromRedirect(QStringLiteral("https://login.microsoftonline.com/")));
        QVERIFY(!openrdp::authorizationCodeFromRedirect(QStringLiteral("https://login.microsoftonline.com/?error=access_denied")));
        QVERIFY(!openrdp::authorizationCodeFromRedirect(QStringLiteral("https://example.test/common/oauth2/nativeclient?code=secret")));
    }
};
QTEST_MAIN(TestCore)
#include "TestCore.moc"
