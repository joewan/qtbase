/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <QtGlobal>
#include <QtAlgorithms>
#include <QtPrintSupport/qprinterinfo.h>

#ifdef Q_OS_UNIX
#  include <unistd.h>
#  include <sys/types.h>
#  include <sys/wait.h>
#endif

Q_DECLARE_METATYPE(QRect)

class tst_QPrinterInfo : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
#ifndef QT_NO_PRINTER
private slots:
    void testForDefaultPrinter();
    void testForPrinters();
    void testForPaperSizes();
    void testConstructors();
    void testAssignment();
    void namedPrinter();

private:
    QString getDefaultPrinterFromSystem();
    QStringList getPrintersFromSystem();

#ifdef Q_OS_UNIX
    QString getOutputFromCommand(const QStringList& command);
#endif // Q_OS_UNIX
#endif // QT_NO_PRINTER
};

void tst_QPrinterInfo::initTestCase()
{
#ifdef QT_NO_PRINTER
    QSKIP("This test requires printing support");
#endif // QT_NO_PRINTER
}

#ifndef QT_NO_PRINTER
QString tst_QPrinterInfo::getDefaultPrinterFromSystem()
{
    QString printer;

#ifdef Q_OS_WIN32
    // TODO "cscript c:\windows\system32\prnmngr.vbs -g"
#endif // Q_OS_WIN32
#ifdef Q_OS_UNIX
    QStringList command;
    command << "lpstat" << "-d";
    QString output = getOutputFromCommand(command);

    QRegExp noDefaultReg("[^:]*no .*default");
    int pos = noDefaultReg.indexIn(output);
    if (pos >= 0) {
        return QString();
    }

    QRegExp defaultReg("default.*: *([a-zA-Z0-9_-]+)");
    defaultReg.indexIn(output);
    printer = defaultReg.cap(1);
#endif // Q_OS_UNIX

    return printer;
}

QStringList tst_QPrinterInfo::getPrintersFromSystem()
{
    QStringList ans;

#ifdef Q_OS_WIN32
    // TODO "cscript c:\windows\system32\prnmngr.vbs -l"
#endif // Q_OS_WIN32
#ifdef Q_OS_UNIX
    QStringList command;
    command << "lpstat" << "-p";
    QString output = getOutputFromCommand(command);
    QStringList list = output.split(QChar::fromLatin1('\n'));

    QRegExp reg("^[Pp]rinter ([.a-zA-Z0-9-_@]+)");
    for (int c = 0; c < list.size(); ++c) {
        if (reg.indexIn(list[c]) >= 0) {
            QString printer = reg.cap(1);
            ans << printer;
        }
    }
#endif // Q_OS_UNIX

    return ans;
}

#ifdef Q_OS_UNIX
// This function does roughly the same as the `command substitution` in
// the shell.
QString tst_QPrinterInfo::getOutputFromCommand(const QStringList& command)
{
// The command execution does nothing on non-unix systems.
    int pid;
    int status = 0;
    int pipePtr[2];

    // Create a pipe that is shared between parent and child process.
    if (pipe(pipePtr) < 0) {
        return QString();
    }
    pid = fork();
    if (pid < 0) {
        close(pipePtr[0]);
        close(pipePtr[1]);
        return QString();
    } else if (pid == 0) {
        // In child.
        // Close the reading end.
        close(pipePtr[0]);
        // Redirect stdout to the pipe.
        if (dup2(pipePtr[1], 1) < 0) {
            exit(1);
        }

        char** argv = new char*[command.size()+1];
        for (int c = 0; c < command.size(); ++c) {
            argv[c] = new char[command[c].size()+1];
            strcpy(argv[c], command[c].toLatin1().data());
        }
        argv[command.size()] = NULL;
        execvp(argv[0], argv);
        // Shouldn't get here, but it's possible if command is not found.
        close(pipePtr[1]);
        close(1);
        for (int c = 0; c < command.size(); ++c) {
            delete [] argv[c];
        }
        delete [] argv;
        exit(1);
    } else {
        // In parent.
        // Close the writing end.
        close(pipePtr[1]);

        QFile pipeRead;
        if (!pipeRead.open(pipePtr[0], QIODevice::ReadOnly)) {
            close(pipePtr[0]);
            return QString();
        }
        QByteArray array;
        array = pipeRead.readAll();
        pipeRead.close();
        close(pipePtr[0]);
        wait(&status);
        return QString(array);
    }
}
#endif

void tst_QPrinterInfo::testForDefaultPrinter()
{
#ifdef Q_OS_WIN32
    QSKIP("Windows test support not yet implemented");
#endif // Q_OS_WIN32
    QString testPrinter = getDefaultPrinterFromSystem();
    QString defaultPrinter = QPrinterInfo::defaultPrinter().printerName();
    QString availablePrinter;
    int availablePrinterDefaults = 0;

    QList<QPrinterInfo> list = QPrinterInfo::availablePrinters();
    for (int c = 0; c < list.size(); ++c) {
        if (list[c].isDefault()) {
            availablePrinter = list.at(c).printerName();
            ++availablePrinterDefaults;
        }
    }

    qDebug() << "Test believes Default Printer                              = " << testPrinter;
    qDebug() << "QPrinterInfo::defaultPrinter() believes Default Printer    = " << defaultPrinter;
    qDebug() << "QPrinterInfo::availablePrinters() believes Default Printer = " << availablePrinter;

    QCOMPARE(testPrinter, defaultPrinter);
    QCOMPARE(testPrinter, availablePrinter);
    if (!availablePrinter.isEmpty())
        QCOMPARE(availablePrinterDefaults, 1);
}

void tst_QPrinterInfo::testForPrinters()
{
#ifdef Q_OS_WIN32
    QSKIP("Windows test support not yet implemented");
#endif // Q_OS_WIN32
    QStringList testPrinters = getPrintersFromSystem();

    QList<QPrinterInfo> printers = QPrinterInfo::availablePrinters();
    QStringList qtPrinters;
    for (int i = 0; i < printers.size(); ++i)
        qtPrinters.append(printers.at(i).printerName());

    qSort(testPrinters);
    qSort(qtPrinters);

    qDebug() << "Test believes Available Printers                              = " << testPrinters;
    qDebug() << "QPrinterInfo::availablePrinters() believes Available Printers = " << qtPrinters;

    QCOMPARE(qtPrinters.size(), testPrinters.size());

    for (int i = 0; i < testPrinters.size(); ++i)
        QCOMPARE(qtPrinters.at(i), testPrinters.at(i));
}

void tst_QPrinterInfo::testForPaperSizes()
{
    // TODO Old PaperSize test dependent on physical printer installed, new generic test required
    // In the meantime just exercise the code path and print-out for inspection.

    QList<QPrinterInfo> printers = QPrinterInfo::availablePrinters();
    for (int i = 0; i < printers.size(); ++i)
        qDebug() << "Printer: " << printers.at(i).printerName() << " Paper Sizes: " << printers.at(i).supportedPaperSizes();
}

void tst_QPrinterInfo::testConstructors()
{
    QPrinterInfo null;
    QCOMPARE(null.printerName(), QString());
    QVERIFY(null.isNull());

    QList<QPrinterInfo> printers = QPrinterInfo::availablePrinters();

    for (int i = 0; i < printers.size(); ++i) {
        QPrinterInfo copy1(printers.at(i));
        QCOMPARE(copy1.printerName(),         printers.at(i).printerName());
        QCOMPARE(copy1.isNull(),              printers.at(i).isNull());
        QCOMPARE(copy1.isDefault(),           printers.at(i).isDefault());
        QCOMPARE(copy1.supportedPaperSizes(), printers.at(i).supportedPaperSizes());

        QPrinter printer(printers.at(i));
        QPrinterInfo copy2(printer);
        QCOMPARE(copy2.printerName(),         printers.at(i).printerName());
        QCOMPARE(copy2.isNull(),              printers.at(i).isNull());
        QCOMPARE(copy2.isDefault(),           printers.at(i).isDefault());
        QCOMPARE(copy2.supportedPaperSizes(), printers.at(i).supportedPaperSizes());
    }
}

void tst_QPrinterInfo::testAssignment()
{
    QList<QPrinterInfo> printers = QPrinterInfo::availablePrinters();

    for (int i = 0; i < printers.size(); ++i) {
        QPrinterInfo copy;
        copy = printers.at(i);
        QCOMPARE(copy.printerName(),         printers.at(i).printerName());
        QCOMPARE(copy.isNull(),              printers.at(i).isNull());
        QCOMPARE(copy.isDefault(),           printers.at(i).isDefault());
        QCOMPARE(copy.supportedPaperSizes(), printers.at(i).supportedPaperSizes());
    }
}

void tst_QPrinterInfo::namedPrinter()
{
    QList<QPrinterInfo> printers = QPrinterInfo::availablePrinters();

    QStringList printerNames;

    foreach (const QPrinterInfo &pi, printers) {
        QPrinterInfo pi2 = QPrinterInfo::printerInfo(pi.printerName());
        qDebug() << "Printer: " << pi2.printerName() << " : "
                 << pi2.isNull() << " : " << pi2.isDefault();
        QCOMPARE(pi2.printerName(),         pi.printerName());
        QCOMPARE(pi2.supportedPaperSizes(), pi.supportedPaperSizes());
        QCOMPARE(pi2.isNull(),              pi.isNull());
        QCOMPARE(pi2.isDefault(),           pi.isDefault());
    }
}

#endif // QT_NO_PRINTER

QTEST_MAIN(tst_QPrinterInfo)
#include "tst_qprinterinfo.moc"
