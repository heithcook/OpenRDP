#include <QtTest>
#include "rdp/RdpInput.h"
#include "rdp/RdpSettings.h"

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
};
QTEST_MAIN(TestCore)
#include "TestCore.moc"
