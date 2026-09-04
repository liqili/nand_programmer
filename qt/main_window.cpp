/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#include "main_window.h"
#include "ui_main_window.h"
#include "settings_programmer_dialog.h"
#include "ecc_settings_dialog.h"
#include "ecc/image_probe.h"
#include "parallel_chip_db_dialog.h"
#include "spi_chip_db_dialog.h"
#include "parallel_serial_chip_db_dialog.h"
#include "firmware_update_dialog.h"
#include "parallel_chip_db.h"
#include "spi_chip_db.h"
#include "logger.h"
#include "about_dialog.h"
#include "settings.h"
#include <QDebug>
#include <QFileDialog>
#include <QFile>
#include <QSettings>
#include <QStringList>
#include <QMessageBox>
#include <memory>
#include <QTimer>
#include <QTime>

#define HEADER_ADDRESS_WIDTH 80
#define HEADER_HEX_WIDTH 340
#define BUFFER_ROW_HEIGHT 20

#define CHIP_NAME_DEFAULT "NONE"
#define CHIP_INDEX_DEFAULT 0
#define CHIP_INDEX2ID(index) (index - 1)
#define CHIP_ID2INDEX(id) (id + 1)

/* Adapter so the image probe can read straight from the work file instead of
 * pulling a whole chip image into memory.
 */
static bool workFileRead(void *ctx, uint64_t offset, uint8_t *buf, size_t len)
{
    QFile *f = static_cast<QFile *>(ctx);

    if (!f->seek(static_cast<qint64>(offset)))
        return false;

    return f->read(reinterpret_cast<char *>(buf),
        static_cast<qint64>(len)) == static_cast<qint64>(len);
}

/* Geometry of the chip the user has selected. spare is what is left of the
 * extended page once the data page is taken off. Returns false when nothing
 * is selected yet.
 */
static bool selectedGeometry(ChipDb *db, const QString &name, int &page,
    int &spare, int &bbMark)
{
    if (!db || name.isEmpty())
        return false;

    const uint32_t p = db->pageSizeGetByName(name);
    const uint32_t e = db->extendedPageSizeGetByName(name);

    if (!p)
        return false;

    page = static_cast<int>(p);
    spare = e > p ? static_cast<int>(e - p) : 0;
    bbMark = db->bbMarkOffsetGetByName(name);

    return true;
}

void MainWindow::initBufTable()
{
#ifdef Q_OS_WIN32
    QFont font("Courier New", 9);
    ui->dataViewer->setFont(font);
#endif
}

void MainWindow::resetBufTable()
{
    ui->dataViewer->setFile(ui->filePathLineEdit->text());
    buffer.buf.clear();
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    Logger *logger = Logger::getInstance();

    ui->setupUi(this);

    logger->setTextEdit(ui->logTextEdit);

    initBufTable();
#ifdef Q_OS_WIN32
    QFont font("Courier New", 9);
    ui->firstSpinBox->setFont(font);
    ui->lastSpinBox->setFont(font);
#endif
    ui->firstSpinBox->setEnabled(false);
    ui->lastSpinBox->setEnabled(false);

    prog = new Programmer(this);
    updateProgSettings();
    updateEccSettings();

    updateChipList();

    connect(ui->chipSelectComboBox, SIGNAL(currentIndexChanged(int)),
        this, SLOT(slotSelectChip(int)));

    connect(ui->actionConnect, SIGNAL(triggered()), this,
        SLOT(slotProgConnect()));
    connect(ui->actionReadId, SIGNAL(triggered()), this,
        SLOT(slotProgReadDeviceId()));
    connect(ui->actionErase, SIGNAL(triggered()), this,
        SLOT(slotProgErase()));
    connect(ui->actionRead, SIGNAL(triggered()), this,
        SLOT(slotProgRead()));
    connect(ui->actionVerify, SIGNAL(triggered()), this,
            SLOT(slotProgVerify()));
    connect(ui->actionWrite, SIGNAL(triggered()), this,
        SLOT(slotProgWrite()));
    connect(ui->actionReadBadBlocks, SIGNAL(triggered()), this,
        SLOT(slotProgReadBadBlocks()));
    connect(ui->actionProgrammer, SIGNAL(triggered()), this,
        SLOT(slotSettingsProgrammer()));
    connect(ui->actionEcc, SIGNAL(triggered()), this,
        SLOT(slotSettingsEcc()));
    connect(ui->actionCheckImage, SIGNAL(triggered()), this,
        SLOT(slotCheckImage()));
    connect(ui->actionParallelChipDb, SIGNAL(triggered()), this,
        SLOT(slotSettingsParallelChipDb()));
    connect(ui->actionSpiChipDb, SIGNAL(triggered()), this,
        SLOT(slotSettingsSpiChipDb()));
    connect(ui->actionParallelSerialChipDb, SIGNAL(triggered()), this,
        SLOT(slotSettingsParallelSerialChipDb()));
    connect(ui->actionAbout, SIGNAL(triggered()), this,
        SLOT(slotAboutDialog()));
    connect(ui->detectPushButton, SIGNAL(clicked()), this,
        SLOT(slotDetectChip()));
    connect(ui->actionFirmwareUpdate, SIGNAL(triggered()), this,
        SLOT(slotFirmwareUpdateDialog()));
    connect(ui->selectFilePushButton, SIGNAL(clicked()), this,
        SLOT(slotSelectFilePath()));
    connect(ui->filePathLineEdit, SIGNAL(editingFinished()), this,
        SLOT(slotFilePathEditingFinished()));

    ui->filePathLineEdit->setText(QDir::tempPath() + "/nando_tmp.bin");
    QSettings settings(SETTINGS_ORGANIZATION_NAME, SETTINGS_APPLICATION_NAME);
    ui->filePathLineEdit->setText(settings.value(SETTINGS_WORK_FILE_PATH,
        ui->filePathLineEdit->text()).toString());
    ui->dataViewer->setFile(ui->filePathLineEdit->text());
}

MainWindow::~MainWindow()
{
    Logger::putInstance();
    delete ui;
}

void MainWindow::setUiStateConnected(bool isConnected)
{
    ui->chipSelectComboBox->setEnabled(isConnected);
    ui->detectPushButton->setEnabled(isConnected);
    ui->actionFirmwareUpdate->setEnabled(isConnected);
    if (!isConnected)
        ui->chipSelectComboBox->setCurrentIndex(CHIP_INDEX_DEFAULT);
}

void MainWindow::setUiStateSelected(bool isSelected)
{
    ui->actionReadId->setEnabled(isSelected);
    ui->actionErase->setEnabled(isSelected);
    ui->actionRead->setEnabled(isSelected);
    ui->actionWrite->setEnabled(isSelected);
    ui->actionVerify->setEnabled(isSelected);
    ui->actionReadBadBlocks->setEnabled(isSelected);
    ui->actionCheckImage->setEnabled(isSelected);

    ui->firstSpinBox->setEnabled(isSelected);
    ui->lastSpinBox->setEnabled(isSelected);
    if (isSelected)
    {
        QString chipName = ui->chipSelectComboBox->currentText();
        quint32 blocksCount = currentChipDb->blockCountGetByName(chipName);
        ui->firstSpinBox->setMaximum(blocksCount - 1);
        ui->firstSpinBox->setValue(0);
        ui->lastSpinBox->setMaximum(blocksCount - 1);
        ui->lastSpinBox->setValue(blocksCount - 1);
        quint64 chipSize = prog->isIncSpare() ?
            currentChipDb->extendedTotalSizeGetByName(chipName) :
            currentChipDb->totalSizeGetByName(chipName);
        ui->blockSizeValueLabel->setText(QString("0x%1").arg(chipSize / blocksCount, 8, 16, QLatin1Char( '0' )));
    }
}

void MainWindow::slotProgConnectCompleted(quint64 status)
{
    disconnect(prog, SIGNAL(connectCompleted(quint64)), this,
        SLOT(slotProgConnectCompleted(quint64)));

    if (status == UINT64_MAX)
        return;

    qInfo() << "Connected to programmer";
    setUiStateConnected(true);
    ui->actionConnect->setText(tr("Disconnect"));
}

void MainWindow::slotProgConnect()
{
    if (!prog->isConnected())
    {
        if (!prog->connect())
        {
            connect(prog, SIGNAL(connectCompleted(quint64)), this,
                SLOT(slotProgConnectCompleted(quint64)));
        }
    }
    else
    {
        prog->disconnect();
        setUiStateConnected(false);
        ui->actionConnect->setText(tr("Connect"));
        qInfo() << "Disconnected from programmer";
    }
}

void MainWindow::slotProgReadDeviceIdCompleted(quint64 status)
{
    QString idStr;

    disconnect(prog, SIGNAL(readChipIdCompleted(quint64)), this,
        SLOT(slotProgReadDeviceIdCompleted(quint64)));

    if (status == UINT64_MAX)
        return;

    idStr = tr("0x%1 0x%2 0x%3 0x%4 0x%5")
        .arg(chipId.makerId, 2, 16, QLatin1Char('0'))
        .arg(chipId.deviceId, 2, 16, QLatin1Char('0'))
        .arg(chipId.thirdId, 2, 16, QLatin1Char('0'))
        .arg(chipId.fourthId, 2, 16, QLatin1Char('0'))
        .arg(chipId.fifthId, 2, 16, QLatin1Char('0'));
    ui->deviceValueLabel->setText(idStr);

    qInfo() << QString("ID ").append(idStr).toLatin1().data();
}

void MainWindow::slotProgReadDeviceId()
{
    qInfo() << "Reading chip ID ...";
    connect(prog, SIGNAL(readChipIdCompleted(quint64)), this,
        SLOT(slotProgReadDeviceIdCompleted(quint64)));
    prog->readChipId(&chipId);
}

void MainWindow::slotProgEraseCompleted(quint64 status)
{
    disconnect(prog, SIGNAL(eraseChipProgress(quint64)), this,
        SLOT(slotProgEraseProgress(quint64)));
    disconnect(prog, SIGNAL(eraseChipCompleted(quint64)), this,
        SLOT(slotProgEraseCompleted(quint64)));

    if (!status)
        qInfo() << "Chip has been erased successfully";

    setProgress(100);
}

void MainWindow::slotProgEraseProgress(quint64 progress)
{
    uint32_t progressPercent;

    progressPercent = progress * 100ULL / areaSize;
    setProgress(progressPercent);
}

void MainWindow::slotProgErase()
{
    quint64 start_address =
            ui->blockSizeValueLabel->text().toULongLong(nullptr, 16)
            * ui->firstSpinBox->value();
    areaSize =
            ui->blockSizeValueLabel->text().toULongLong(nullptr, 16)
            * (ui->lastSpinBox->value() + 1) - start_address;

    if (!areaSize)
    {
        qCritical() << "Chip size is not set";
        return;
    }

    qInfo() << "Erasing chip ...";

    setProgress(0);

    connect(prog, SIGNAL(eraseChipCompleted(quint64)), this,
        SLOT(slotProgEraseCompleted(quint64)));
    connect(prog, SIGNAL(eraseChipProgress(quint64)), this,
        SLOT(slotProgEraseProgress(quint64)));

    prog->eraseChip(start_address, areaSize);
}

EccMode MainWindow::readEccMode() const
{
    if (!eccEngine.isEnabled())
        return ECC_MODE_RAW;

    return eccCorrectOnRead ? ECC_MODE_CORRECT : ECC_MODE_CHECK;
}

void MainWindow::flushReadBuffer(bool final)
{
    std::vector<uint8_t> out;

    buffer.mutex.lock();
    if (!buffer.buf.empty())
    {
        eccStream.push(&buffer.buf[0], buffer.buf.size(), out, eccStats);
        buffer.buf.clear();
    }
    buffer.mutex.unlock();

    if (final)
        eccStream.end(out, eccStats);

    if (!out.empty())
    {
        workFile.write(reinterpret_cast<const char *>(&out[0]),
            static_cast<qint64>(out.size()));
    }
}

void MainWindow::reportEccStats()
{
    if (!eccStats.stepsTotal)
        return;

    qInfo() << QString::fromStdString(eccStats.summary());

    if (eccStats.stepsUncorrectable)
    {
        const QString msg = tr("ECC: %1 step(s) could not be corrected, first "
            "at page %2").arg(eccStats.stepsUncorrectable)
            .arg(eccStats.firstFailPage);

        if (eccWarnUncorrectable)
            qCritical() << msg;
        else
            qInfo() << msg;
    }
    else if (eccStats.wearWarning(eccEngine.scheme()))
    {
        /* Still readable, but one more flip in the same step would not be.
         * Worth saying before the data is actually lost.
         */
        qWarning() << tr("ECC: worst step needed %1 of %2 correctable bits. "
            "These blocks are wearing out; consider rewriting them.")
            .arg(eccStats.worstStep).arg(eccEngine.scheme().correctableBits());
    }
}

void MainWindow::slotProgReadCompleted(quint64 readBytes)
{
    disconnect(prog, SIGNAL(readChipProgress(quint64)), this,
        SLOT(slotProgReadProgress(quint64)));
    disconnect(prog, SIGNAL(readChipCompleted(quint64)), this,
        SLOT(slotProgReadCompleted(quint64)));

    ui->filePathLineEdit->setDisabled(false);
    ui->selectFilePushButton->setDisabled(false);

    setProgress(100);

    if (readBytes == UINT64_MAX)
    {
        workFile.close();
        return;
    }

    flushReadBuffer(true);
    reportEccStats();

    if (readBytes != (quint64)workFile.size())
    {
        qCritical() << "Read operation returned more or less than requested: " <<
            readBytes << "!=" << workFile.size();
        workFile.resize(0);
    }

    workFile.close();
    qInfo() << "Data has been successfully read";
    ui->dataViewer->setFile(ui->filePathLineEdit->text());
}

void MainWindow::slotProgReadProgress(quint64 progress)
{
    uint32_t progressPercent;

    progressPercent = progress * 100ULL / areaSize;
    setProgress(progressPercent);

    flushReadBuffer(false);
}

void MainWindow::slotProgRead()
{
    quint64 start_address =
            ui->blockSizeValueLabel->text().toULongLong(nullptr, 16)
            * ui->firstSpinBox->value();
    areaSize  =
            ui->blockSizeValueLabel->text().toULongLong(nullptr, 16)
            * (ui->lastSpinBox->value() + 1) - start_address;

    if (!areaSize)
    {
        qCritical() << "Chip size is not set";
        return;
    }

    workFile.setFileName(ui->filePathLineEdit->text());
    if (workFile.exists())
    {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("Replace data in current file?");
        msgBox.setInformativeText("Selected file name is exist.");
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        if (msgBox.exec() == QMessageBox::Cancel)
        {
            workFile.close();
            return;
        }
    }

    if (!workFile.open(QIODevice::WriteOnly))
    {
        qCritical() << "Failed to open file:" << ui->filePathLineEdit->text()
            << ", error:" << workFile.errorString();
        return;
    }

    workFile.resize(0);
    resetBufTable();
    buffer.buf.clear();

    qInfo() << "Reading data ...";
    setProgress(0);

    /* Arm the ECC stream for this transfer. With no scheme bound, or with
     * correction switched off, this is a pass through and the file receives
     * exactly what the chip returned.
     */
    eccStats.reset();
    eccStream.begin(&eccEngine, eccEngine.pageBytes(), readEccMode());

    if (eccEngine.isEnabled())
    {
        qInfo() << (eccCorrectOnRead ?
            "ECC: checking and correcting -" : "ECC: checking only -") <<
            QString::fromStdString(eccEngine.strengthText());
    }

    connect(prog, SIGNAL(readChipCompleted(quint64)), this,
        SLOT(slotProgReadCompleted(quint64)));
    connect(prog, SIGNAL(readChipProgress(quint64)), this,
        SLOT(slotProgReadProgress(quint64)));

    ui->filePathLineEdit->setDisabled(true);
    ui->selectFilePushButton->setDisabled(true);

    prog->readChip(&buffer, start_address, areaSize, true);
}

void MainWindow::slotProgVerifyCompleted(quint64 readBytes)
{
    disconnect(prog, SIGNAL(readChipProgress(quint64)), this,
               SLOT(slotProgVerifyProgress(quint64)));
    disconnect(prog, SIGNAL(readChipCompleted(quint64)), this,
               SLOT(slotProgVerifyCompleted(quint64)));

    ui->filePathLineEdit->setDisabled(false);
    ui->selectFilePushButton->setDisabled(false);

    setProgress(100);
    workFile.close();
    buffer.buf.clear();

    if (eccStats.stepsTotal)
    {
        qInfo() << "Verify is byte exact; the ECC figures below describe the "
            "state of the data on the chip, they do not change the verdict.";
        reportEccStats();
    }

    qInfo() << readBytes << " bytes read. Verify end."  ;
}

void MainWindow::slotProgVerifyProgress(quint64 progress)
{
    uint32_t progressPercent;

    progressPercent = progress * 100ULL / areaSize;
    setProgress(progressPercent);

    QVector<uint8_t> cmpBuffer;
    buffer.mutex.lock();
    cmpBuffer.resize(buffer.buf.size());

    /* Run the parity check over what the chip returned, for reporting only.
     * The verdict below stays a byte for byte comparison: a page that was
     * written badly but is still correctable must not be allowed to compare
     * equal, or the write failure is hidden.
     */
    if (!buffer.buf.empty())
    {
        std::vector<uint8_t> discard;
        eccStream.push(&buffer.buf[0], buffer.buf.size(), discard, eccStats);
    }

    qint64 readSize = workFile.read((char *)cmpBuffer.data(), buffer.buf.size());

    if (readSize < 0)
    {
        qCritical() << "Failed to read file";
    }
    else if (readSize == 0)
    {
        qCritical() << "File read 0 byte";
    }

    for(uint32_t i = 0; i < readSize; i++)
    {
        if(cmpBuffer.at(i) != buffer.buf.at(i))
        {
            uint64_t block = progress / ui->blockSizeValueLabel->text().toULongLong(nullptr, 16)
                             + ui->firstSpinBox->text().toULongLong(nullptr, 10) - 1;
            uint64_t byte = progress - ui->blockSizeValueLabel->text().toULongLong(nullptr, 16)
                            + ui->firstSpinBox->text().toULongLong(nullptr, 10)
                            * ui->blockSizeValueLabel->text().toULongLong(nullptr, 16) + i;
            qCritical() << "Wrong block: " << QString("%1").arg(block)
                << ", Wrong byte addr: "
                << QString("0x%1").arg(byte, 8, 16, QLatin1Char( '0' ));
            break;
        }
    }

    buffer.buf.clear();
    buffer.mutex.unlock();
}

void MainWindow::slotProgVerify()
{
    int index;
    QString chipName;

    workFile.setFileName(ui->filePathLineEdit->text());
    if (!workFile.open(QIODevice::ReadOnly))
    {
        qCritical() << "Failed to open compare file:" << ui->filePathLineEdit->text() << ", error:" <<
            workFile.errorString();
        return;
    }
    if (!workFile.size())
    {
        qInfo() << "Compare file is empty";
        return;
    }

    index = ui->chipSelectComboBox->currentIndex();
    if (index <= CHIP_INDEX_DEFAULT)
    {
        qInfo() << "Chip is not selected";
        return;
    }

    chipName = ui->chipSelectComboBox->currentText();
    pageSize = prog->isIncSpare() ?
                   currentChipDb->extendedPageSizeGetByName(chipName) :
                   currentChipDb->pageSizeGetByName(chipName);
    if (!pageSize)
    {
        qInfo() << "Chip page size is unknown";
        return;
    }

    quint64 start_address =
        ui->blockSizeValueLabel->text().toULongLong(nullptr, 16)
        * ui->firstSpinBox->value();

    areaSize = workFile.size();

    if (areaSize % pageSize)
    {
        areaSize = (areaSize / pageSize + 1) * pageSize;
    }

    quint64 setSize =
        ui->blockSizeValueLabel->text().toULongLong(nullptr, 16)
            * (ui->lastSpinBox->value() + 1) - start_address;

    if (setSize < areaSize)
        areaSize = setSize;

    qInfo() << "Reading data ...";
    setProgress(0);

    /* Always CHECK here, never CORRECT: verify compares what is actually on
     * the chip, so repairing the bytes first would compare a repaired copy
     * against the file and pass a page that was written badly.
     */
    eccStats.reset();
    eccStream.begin(&eccEngine, eccEngine.pageBytes(), ECC_MODE_CHECK);

    connect(prog, SIGNAL(readChipCompleted(quint64)), this,
            SLOT(slotProgVerifyCompleted(quint64)));
    connect(prog, SIGNAL(readChipProgress(quint64)), this,
            SLOT(slotProgVerifyProgress(quint64)));

    ui->filePathLineEdit->setDisabled(true);
    ui->selectFilePushButton->setDisabled(true);

    buffer.buf.clear();

    prog->readChip(&buffer, start_address, areaSize, true);
}

void MainWindow::slotProgWriteCompleted(int status)
{
    disconnect(prog, SIGNAL(writeChipProgress(quint64)), this,
        SLOT(slotProgWriteProgress(quint64)));
    disconnect(prog, SIGNAL(writeChipCompleted(int)), this,
        SLOT(slotProgWriteCompleted(int)));

    ui->filePathLineEdit->setDisabled(false);
    ui->selectFilePushButton->setDisabled(false);

    if (!status)
        qInfo() << "Data has been successfully written";

    setProgress(100);
    workFile.close();
}

void MainWindow::slotProgWriteProgress(quint64 progress)
{
    uint32_t progressPercent;

    progressPercent = progress * 100ULL / areaSize;
    setProgress(progressPercent);

    std::unique_lock<std::mutex> lck(buffer.mutex);

    qint64 readSize = workFile.read((char *)buffer.buf.data(), pageSize);
    if (readSize < 0)
    {
        qCritical() << "Failed to read file";
        return;
    }
    else if (readSize == 0)
    {
        return;
    }
    else if (readSize < pageSize)
    {
        std::fill(buffer.buf.begin() + readSize, buffer.buf.end(), 0xFF);
    }

    // Notify writer that new data is ready
    buffer.ready = true;
    buffer.cv.notify_one();
}

void MainWindow::slotProgWrite()
{
    int index;
    QString chipName;

    workFile.setFileName(ui->filePathLineEdit->text());
    if (!workFile.open(QIODevice::ReadOnly))
    {
        qCritical() << "Failed to open file:" << ui->filePathLineEdit->text() << ", error:" <<
            workFile.errorString();
        return;
    }
    if (!workFile.size())
    {
        qInfo() << "Write file is empty";
        return;
    }

    index = ui->chipSelectComboBox->currentIndex();
    if (index <= CHIP_INDEX_DEFAULT)
    {
        qInfo() << "Chip is not selected";
        return;
    }

    chipName = ui->chipSelectComboBox->currentText();
    pageSize = prog->isIncSpare() ?
        currentChipDb->extendedPageSizeGetByName(chipName) :
        currentChipDb->pageSizeGetByName(chipName);
    if (!pageSize)
    {
        qInfo() << "Chip page size is unknown";
        return;
    }

    quint64 start_address =
            ui->blockSizeValueLabel->text().toULongLong(nullptr, 16)
            * ui->firstSpinBox->value();

    /* Work out what the image actually is before sending any of it. Writing a
     * data only image while the transfer carries the spare area shifts every
     * page by the spare size: data lands in spare areas, bad block markers are
     * overwritten, and the chip ends up garbage with no warning at any point.
     */
    {
        int dataPage = 0, spare = 0, bbMark = -1;

        if (selectedGeometry(currentChipDb, chipName, dataPage, spare, bbMark)
            && spare > 0)
        {
            const ImageProbeResult probe = ImageProbe::probe(workFileRead,
                &workFile, static_cast<quint64>(workFile.size()), dataPage,
                spare, bbMark);
            const bool haveSpare = probe.layout == IMAGE_LAYOUT_PAGE_SPARE;

            workFile.seek(0);
            qInfo() << "Image:" << QString::fromStdString(probe.reason);

            if (prog->isIncSpare() && !haveSpare)
            {
                qCritical() << tr("This image has no spare area, but the "
                    "transfer includes it. Writing it would shift every page "
                    "by %1 bytes and destroy the bad block markers. Generate "
                    "the spare area first, or turn off \"Include spare "
                    "area\".").arg(spare);
                workFile.close();
                return;
            }

            if (!prog->isIncSpare() && haveSpare)
            {
                qCritical() << tr("This image contains a spare area, but the "
                    "transfer does not. Enable \"Include spare area\" in "
                    "Programmer settings before writing it.");
                workFile.close();
                return;
            }

            if (probe.foreignBadMarkers)
            {
                /* Not fatal, but it marks good blocks bad on the destination
                 * and cannot be undone, so it must not pass silently.
                 */
                qWarning() << tr("Image carries %1 bad block marker(s) from "
                    "the device it was taken from. Writing it will mark those "
                    "blocks bad on this chip.")
                    .arg(probe.foreignBadMarkers);
            }
        }
    }

    areaSize = workFile.size();

    if (areaSize % pageSize)
    {
        areaSize = (areaSize / pageSize + 1) * pageSize;
    }

    quint64 setSize =
            ui->blockSizeValueLabel->text().toULongLong(nullptr, 16)
            * (ui->lastSpinBox->value() + 1) - start_address;

    if (setSize < areaSize)
        areaSize = setSize;

    qInfo() << "Writing data ...";

    connect(prog, SIGNAL(writeChipCompleted(int)), this,
        SLOT(slotProgWriteCompleted(int)));
    connect(prog, SIGNAL(writeChipProgress(quint64)), this,
        SLOT(slotProgWriteProgress(quint64)));

    ui->filePathLineEdit->setDisabled(true);
    ui->selectFilePushButton->setDisabled(true);

    buffer.buf.reserve(pageSize);
    buffer.buf.resize(pageSize);
    qint64 readSize = workFile.read((char *)buffer.buf.data(), (qint64)pageSize);
    if (readSize < 0)
    {
        qCritical() << "Failed to read file";
        return;
    }
    else if (readSize == 0)
    {
        qInfo() << "File is empty";
        return;
    }
    else if (readSize < pageSize)
    {
        std::fill(buffer.buf.begin() + readSize, buffer.buf.end(), 0xFF);
    }

    buffer.ready = true;
    prog->writeChip(&buffer, start_address, areaSize, pageSize);
}

void MainWindow::slotProgReadBadBlocksCompleted(quint64 status)
{
    disconnect(prog, SIGNAL(readChipBadBlocksCompleted(quint64)), this,
        SLOT(slotProgReadBadBlocksCompleted(quint64)));
    disconnect(prog, SIGNAL(readChipBadBlocksProgress(quint64)), this,
        SLOT(slotProgReadBadBlocksProgress(quint64)));

    if (!status)
        qInfo() << "Bad blocks have been successfully read";

    setProgress(100);
}

void MainWindow::slotProgReadBadBlocksProgress(quint64 progress)
{
    uint32_t progressPercent;
    QString chipName = ui->chipSelectComboBox->currentText();
    quint64 pageNum =
        currentChipDb->extendedTotalSizeGetByName(chipName) /
        currentChipDb->extendedPageSizeGetByName(chipName);

    progressPercent = progress * 100ULL / pageNum;
    setProgress(progressPercent);
}

void MainWindow::slotProgReadBadBlocks()
{
    qInfo() << "Reading bad blocks ...";

    connect(prog, SIGNAL(readChipBadBlocksCompleted(quint64)), this,
        SLOT(slotProgReadBadBlocksCompleted(quint64)));
    connect(prog, SIGNAL(readChipBadBlocksProgress(quint64)), this,
        SLOT(slotProgReadBadBlocksProgress(quint64)));

    prog->readChipBadBlocks();
}

void MainWindow::slotProgSelectCompleted(quint64 status)
{
    disconnect(prog, SIGNAL(confChipCompleted(quint64)), this,
        SLOT(slotProgSelectCompleted(quint64)));

    if (!status)
    {
        setUiStateSelected(true);
        qInfo() << "Programmer configured successfully";
    }
    else
        setUiStateSelected(false);
}

void MainWindow::slotSelectChip(int selectedChipNum)
{
    QString name;
    ChipInfo *chipInfo;

    if (selectedChipNum <= CHIP_INDEX_DEFAULT)
    {
        setUiStateSelected(false);
        return;
    }

    name = ui->chipSelectComboBox->currentText();
    if (name.isEmpty())
    {
        qCritical() << "Failed to get chip name";
        return;
    }

    if ((chipInfo = parallelChipDb.chipInfoGetByName(name)))
        currentChipDb = &parallelChipDb;
    else if ((chipInfo = spiChipDb.chipInfoGetByName(name)))
        currentChipDb = &spiChipDb;
    else if ((chipInfo = parallelSerialChipDb.chipInfoGetByName(name)))
        currentChipDb = &parallelSerialChipDb;
    else
    {
        qCritical() << "Failed to find chip in DB";
        return;
    }

    qInfo() << "Configuring programmer ...";

    connect(prog, SIGNAL(confChipCompleted(quint64)), this,
        SLOT(slotProgSelectCompleted(quint64)));

    if (chipInfo)
        prog->confChip(chipInfo);
}

void MainWindow::detectChipDelayed()
{

    if (currentChipDb == &spiChipDb)
    {
        // Search in next DB
        detectChip(&parallelSerialChipDb);
    }
    else if (currentChipDb == &parallelSerialChipDb)
        qInfo() << "Chip not found in database";
    else
    {
        // Search in next DB
        detectChip(&spiChipDb);
    }
}

void MainWindow::setChipNameDelayed()
{
    QString chipName = currentChipDb->getNameByChipId(chipId.makerId,
        chipId.deviceId, chipId.thirdId, chipId.fourthId, chipId.fifthId);

    for (int i = 0; i < ui->chipSelectComboBox->count(); i++)
    {
        if (!ui->chipSelectComboBox->itemText(i).compare(chipName))
            ui->chipSelectComboBox->setCurrentIndex(i);
    }
}

void MainWindow::slotProgDetectChipReadChipIdCompleted(quint64 status)
{
    QString idStr;
    QString chipName;

    disconnect(prog, SIGNAL(readChipIdCompleted(quint64)), this,
        SLOT(slotProgDetectChipReadChipIdCompleted(quint64)));

    if (status == UINT64_MAX)
        return;

    idStr = tr("0x%1 0x%2 0x%3 0x%4 0x%5")
        .arg(chipId.makerId, 2, 16, QLatin1Char('0'))
        .arg(chipId.deviceId, 2, 16, QLatin1Char('0'))
        .arg(chipId.thirdId, 2, 16, QLatin1Char('0'))
        .arg(chipId.fourthId, 2, 16, QLatin1Char('0'))
        .arg(chipId.fifthId, 2, 16, QLatin1Char('0'));

    ui->deviceValueLabel->setText(idStr);

    qInfo() << QString("ID ").append(idStr).toLatin1().data();

    chipName = currentChipDb->getNameByChipId(chipId.makerId, chipId.deviceId,
        chipId.thirdId, chipId.fourthId, chipId.fifthId);

    if (chipName.isEmpty())
    {
        QTimer::singleShot(50, this, &MainWindow::detectChipDelayed);
        return;
    }

    QTimer::singleShot(50, this, &MainWindow::setChipNameDelayed);
}

void MainWindow::detectChipReadChipIdDelayed()
{
    connect(prog, SIGNAL(readChipIdCompleted(quint64)), this,
        SLOT(slotProgDetectChipReadChipIdCompleted(quint64)));
    prog->readChipId(&chipId);
}

void MainWindow::slotProgDetectChipConfCompleted(quint64 status)
{
    disconnect(prog, SIGNAL(confChipCompleted(quint64)), this,
        SLOT(slotProgDetectChipConfCompleted(quint64)));

    if (status == UINT64_MAX)
        return;

    QTimer::singleShot(50, this, &MainWindow::detectChipReadChipIdDelayed);
}

void MainWindow::detectChip(ChipDb *chipDb)
{
    ChipInfo *chipInfo;


    currentChipDb = chipDb;

    // Assuming read of ID is the same for all chips thereby use settings of the
    // first one.
    if (!(chipInfo = currentChipDb->chipInfoGetById(0)))
    {
        qCritical() << "Failed to get information from chip database";
        return;
    }

    connect(prog, SIGNAL(confChipCompleted(quint64)), this,
        SLOT(slotProgDetectChipConfCompleted(quint64)));
    prog->confChip(chipInfo);
}

void MainWindow::slotDetectChip()
{
    qInfo() << "Detecting chip ...";

    detectChip(&parallelChipDb);
}

void MainWindow::slotSettingsProgrammer()
{
    SettingsProgrammerDialog progDialog(this);
    QSettings settings(SETTINGS_ORGANIZATION_NAME, SETTINGS_APPLICATION_NAME);

    progDialog.setUsbDevName(settings.value(SETTINGS_USB_DEV_NAME,
        prog->getUsbDevName()).toString());
    progDialog.setSkipBB((settings.value(SETTINGS_SKIP_BAD_BLOCKS,
        prog->isSkipBB())).toBool());
    progDialog.setIncSpare((settings.value(SETTINGS_INCLUDE_SPARE_AREA,
        prog->isIncSpare())).toBool());
    progDialog.setHwEccEnabled((settings.value(SETTINGS_ENABLE_HW_ECC,
        prog->isHwEccEnabled())).toBool());
    progDialog.setAlertEnabled((settings.value(SETTINGS_ENABLE_ALERT,
        isAlertEnabled)).toBool());

    if (progDialog.exec() == QDialog::Accepted)
    {
        settings.setValue(SETTINGS_USB_DEV_NAME, progDialog.getUsbDevName());
        settings.setValue(SETTINGS_SKIP_BAD_BLOCKS, progDialog.isSkipBB());
        settings.setValue(SETTINGS_INCLUDE_SPARE_AREA, progDialog.isIncSpare());
        settings.setValue(SETTINGS_ENABLE_HW_ECC, progDialog.isHwEccEnabled());
        settings.setValue(SETTINGS_ENABLE_ALERT, progDialog.isAlertEnabled());
        settings.sync();

        updateProgSettings();
    }
}

/* Examine the file the user has selected without touching the device: what
 * layout it is in, whether it already carries parity, which scheme that parity
 * belongs to, and how healthy it is. Useful before writing, and useful on its
 * own for judging a dump.
 */
void MainWindow::slotCheckImage()
{
    QFile file(ui->filePathLineEdit->text());
    int dataPage = 0, spare = 0, bbMark = -1;

    if (ui->chipSelectComboBox->currentIndex() <= CHIP_INDEX_DEFAULT ||
        !selectedGeometry(currentChipDb, ui->chipSelectComboBox->currentText(),
        dataPage, spare, bbMark))
    {
        qInfo() << "Select a chip first: the check needs its page geometry";
        return;
    }

    if (!file.open(QIODevice::ReadOnly) || !file.size())
    {
        qCritical() << "Cannot read" << file.fileName();
        return;
    }

    const ImageProbeResult probe = ImageProbe::probe(workFileRead, &file,
        static_cast<quint64>(file.size()), dataPage, spare, bbMark);

    qInfo() << "Image:" << file.fileName() << file.size() << "bytes";
    qInfo() << "Layout:" << (probe.layout == IMAGE_LAYOUT_PAGE_SPARE ?
        "page + spare" : probe.layout == IMAGE_LAYOUT_DATA_ONLY ?
        "data only, no spare area" : "unrecognised") <<
        QString("(confidence %1%)").arg(probe.confidence);
    qInfo() << QString::fromStdString(probe.reason);
    qInfo() << QString("%1 whole page(s)%2").arg(probe.fullPages)
        .arg(probe.trailingBytes ?
            QString(", plus %1 trailing byte(s)").arg(probe.trailingBytes) :
            QString());

    if (probe.foreignBadMarkers)
    {
        qWarning() << QString("Carries %1 bad block marker(s) from the device "
            "it came from; writing it would mark those blocks bad here.")
            .arg(probe.foreignBadMarkers);
    }

    if (probe.layout != IMAGE_LAYOUT_PAGE_SPARE)
    {
        file.close();
        return;
    }

    if (probe.spareBlank)
    {
        qInfo() << "Spare area is present but empty; no parity to check.";
        file.close();
        return;
    }

    /* Check the whole file with whichever scheme was recognised, falling back
     * to the one configured in the ECC dialog.
     */
    EccEngine eng;
    std::string why;
    const EccScheme s = probe.matchingPreset >= 0 ?
        EccScheme::preset(probe.matchingPreset) : eccScheme;

    if (s.algo == ECC_ALGO_NONE ||
        !eng.setScheme(s, dataPage, spare, bbMark, why))
    {
        qInfo() << "No ECC scheme to check with." <<
            (why.empty() ? "" : QString::fromStdString(why));
        file.close();
        return;
    }

    EccPageStream stream;
    EccStats stats;
    std::vector<uint8_t> chunk(64 * 1024), out;

    stream.begin(&eng, eng.pageBytes(), ECC_MODE_CHECK);

    for (;;)
    {
        const qint64 n = file.read(reinterpret_cast<char *>(&chunk[0]),
            static_cast<qint64>(chunk.size()));

        if (n <= 0)
            break;

        out.clear();
        stream.push(&chunk[0], static_cast<size_t>(n), out, stats);
    }

    out.clear();
    stream.end(out, stats);
    file.close();

    qInfo() << QString::fromStdString(stats.summary());

    if (stats.stepsUncorrectable)
    {
        qCritical() << QString("%1 step(s) are beyond repair, first at page %2")
            .arg(stats.stepsUncorrectable).arg(stats.firstFailPage);
    }
    else if (stats.wearWarning(s))
    {
        qWarning() << QString("Worst step needed %1 of %2 correctable bits; "
            "this image came off blocks that are wearing out.")
            .arg(stats.worstStep).arg(s.correctableBits());
    }
}

void MainWindow::slotSettingsEcc()
{
    EccSettingsDialog eccDialog(this);
    QSettings settings(SETTINGS_ORGANIZATION_NAME, SETTINGS_APPLICATION_NAME);
    int page = 0, spare = 0, bbMark = -1;

    if (ui->chipSelectComboBox->currentIndex() > 0 &&
        selectedGeometry(currentChipDb, ui->chipSelectComboBox->currentText(),
        page, spare, bbMark))
    {
        eccDialog.setChipGeometry(page, spare, bbMark);
    }

    eccDialog.setSpareIncluded(prog->isIncSpare());

    {
    }

    /* Start from the stored layout, falling back to the Broadcom BCH-4 preset
     * the first time the dialog is opened.
     */
    EccScheme s = EccScheme::preset(1);

    s.algo = static_cast<EccAlgorithm>(settings.value(SETTINGS_ECC_ALGO,
        s.algo).toInt());
    s.sectorSize = settings.value(SETTINGS_ECC_SECTOR_SIZE,
        s.sectorSize).toInt();
    s.oobSize = settings.value(SETTINGS_ECC_OOB_SIZE, s.oobSize).toInt();
    s.eccBitOffset = settings.value(SETTINGS_ECC_BIT_OFFSET,
        s.eccBitOffset).toInt();
    s.coverSpare = settings.value(SETTINGS_ECC_COVER_SPARE,
        s.coverSpare).toBool();
    s.m = settings.value(SETTINGS_ECC_M, s.m).toInt();
    s.t = settings.value(SETTINGS_ECC_T, s.t).toInt();
    s.primPoly = settings.value(SETTINGS_ECC_PRIM_POLY, s.primPoly).toUInt();
    s.truncateTopBits = settings.value(SETTINGS_ECC_TRUNCATE,
        s.truncateTopBits).toInt();

    eccDialog.setScheme(s);
    eccDialog.setEccEnabled(settings.value(SETTINGS_ECC_ENABLED,
        false).toBool());
    eccDialog.setCorrectOnRead(settings.value(SETTINGS_ECC_CORRECT_ON_READ,
        true).toBool());
    eccDialog.setGenerateOnWrite(settings.value(SETTINGS_ECC_GENERATE_ON_WRITE,
        true).toBool());
    eccDialog.setStopOnUncorrectable(settings.value(
        SETTINGS_ECC_WARN_UNCORRECTABLE, true).toBool());

    if (eccDialog.exec() == QDialog::Accepted)
    {
        const EccScheme n = eccDialog.scheme();

        settings.setValue(SETTINGS_ECC_ENABLED, eccDialog.isEccEnabled());
        settings.setValue(SETTINGS_ECC_ALGO, static_cast<int>(n.algo));
        settings.setValue(SETTINGS_ECC_SECTOR_SIZE, n.sectorSize);
        settings.setValue(SETTINGS_ECC_OOB_SIZE, n.oobSize);
        settings.setValue(SETTINGS_ECC_BIT_OFFSET, n.eccBitOffset);
        settings.setValue(SETTINGS_ECC_COVER_SPARE, n.coverSpare);
        settings.setValue(SETTINGS_ECC_M, n.m);
        settings.setValue(SETTINGS_ECC_T, n.t);
        settings.setValue(SETTINGS_ECC_PRIM_POLY, n.primPoly);
        settings.setValue(SETTINGS_ECC_TRUNCATE, n.truncateTopBits);
        settings.setValue(SETTINGS_ECC_CORRECT_ON_READ,
            eccDialog.isCorrectOnRead());
        settings.setValue(SETTINGS_ECC_GENERATE_ON_WRITE,
            eccDialog.isGenerateOnWrite());
        settings.setValue(SETTINGS_ECC_WARN_UNCORRECTABLE,
            eccDialog.isStopOnUncorrectable());
        settings.sync();

        updateEccSettings();
    }
}

void MainWindow::updateEccSettings()
{
    QSettings settings(SETTINGS_ORGANIZATION_NAME, SETTINGS_APPLICATION_NAME);
    EccScheme s = EccScheme::preset(1);

    s.algo = static_cast<EccAlgorithm>(settings.value(SETTINGS_ECC_ALGO,
        s.algo).toInt());
    s.sectorSize = settings.value(SETTINGS_ECC_SECTOR_SIZE,
        s.sectorSize).toInt();
    s.oobSize = settings.value(SETTINGS_ECC_OOB_SIZE, s.oobSize).toInt();
    s.eccBitOffset = settings.value(SETTINGS_ECC_BIT_OFFSET,
        s.eccBitOffset).toInt();
    s.coverSpare = settings.value(SETTINGS_ECC_COVER_SPARE,
        s.coverSpare).toBool();
    s.m = settings.value(SETTINGS_ECC_M, s.m).toInt();
    s.t = settings.value(SETTINGS_ECC_T, s.t).toInt();
    s.primPoly = settings.value(SETTINGS_ECC_PRIM_POLY, s.primPoly).toUInt();
    s.truncateTopBits = settings.value(SETTINGS_ECC_TRUNCATE,
        s.truncateTopBits).toInt();

    /* The layout is kept as configured even when ECC is switched off, so the
     * user does not have to type it again; only the effective scheme handed to
     * the engine is neutered.
     */
    if (!settings.value(SETTINGS_ECC_ENABLED, false).toBool())
        s.algo = ECC_ALGO_NONE;

    eccScheme = s;
    eccCorrectOnRead = settings.value(SETTINGS_ECC_CORRECT_ON_READ,
        true).toBool();
    eccGenerateOnWrite = settings.value(SETTINGS_ECC_GENERATE_ON_WRITE,
        true).toBool();
    eccWarnUncorrectable = settings.value(SETTINGS_ECC_WARN_UNCORRECTABLE,
        true).toBool();

    rebindEcc();
}

bool MainWindow::rebindEcc()
{
    std::string why;
    int page = 0, spare = 0, bbMark = -1;

    if (eccScheme.algo == ECC_ALGO_NONE ||
        ui->chipSelectComboBox->currentIndex() <= 0 ||
        !selectedGeometry(currentChipDb, ui->chipSelectComboBox->currentText(),
        page, spare, bbMark))
    {
        eccEngine = EccEngine();
        return true;
    }

    /* Parity lives in the spare area, so the host has to be receiving it. With
     * "include spare area" off the transfer carries data bytes only, and
     * anything the engine did would be computed over the wrong bytes.
     */
    if (!prog->isIncSpare())
    {
        eccEngine = EccEngine();
        qInfo() << "ECC not applied: enable \"Include spare area\" in "
            "Programmer settings, the parity lives there";
        return false;
    }

    if (!eccEngine.setScheme(eccScheme, page, spare, bbMark, why))
    {
        /* Refuse to apply a layout that does not fit rather than silently
         * mangling the spare area of a real device.
         */
        eccEngine = EccEngine();
        qInfo() << "ECC not applied to this chip:" <<
            QString::fromStdString(why);
        return false;
    }

    qInfo() << "ECC:" << QString::fromStdString(eccEngine.strengthText()) <<
        "over" << eccEngine.sectorsPerPage() << "sectors per page";

    return true;
}

void MainWindow::updateProgSettings()
{
    QSettings settings(SETTINGS_ORGANIZATION_NAME, SETTINGS_APPLICATION_NAME);

    if (settings.contains(SETTINGS_USB_DEV_NAME))
        prog->setUsbDevName(settings.value(SETTINGS_USB_DEV_NAME).toString());
    if (settings.contains(SETTINGS_SKIP_BAD_BLOCKS))
        prog->setSkipBB(settings.value(SETTINGS_SKIP_BAD_BLOCKS).toBool());
    if (settings.contains(SETTINGS_INCLUDE_SPARE_AREA))
    {
        prog->setIncSpare(settings.value(SETTINGS_INCLUDE_SPARE_AREA).
            toBool());
    }
    if (settings.contains(SETTINGS_ENABLE_HW_ECC))
        prog->setHwEccEnabled(settings.value(SETTINGS_ENABLE_HW_ECC).toBool());
    if (settings.contains(SETTINGS_ENABLE_ALERT))
        isAlertEnabled = settings.value(SETTINGS_ENABLE_ALERT).toBool();

    if (ui->chipSelectComboBox->currentIndex() > 0)
    {
        setUiStateSelected(true);
    }
}

void MainWindow::slotSettingsParallelChipDb()
{
    ParallelChipDbDialog chipDbDialog(&parallelChipDb, this);

    if (chipDbDialog.exec() == QDialog::Accepted)
        updateChipList();
}

void MainWindow::slotSettingsSpiChipDb()
{
    SpiChipDbDialog chipDbDialog(&spiChipDb, this);

    if (chipDbDialog.exec() == QDialog::Accepted)
        updateChipList();
}

void MainWindow::slotSettingsParallelSerialChipDb()
{
    ParallelSerialChipDbDialog chipDbDialog(&parallelSerialChipDb, this);

    if (chipDbDialog.exec() == QDialog::Accepted)
        updateChipList();
}

void MainWindow::updateChipList()
{
    int i = 0;
    QStringList chipNames;

    ui->chipSelectComboBox->clear();
    ui->chipSelectComboBox->addItem(CHIP_NAME_DEFAULT);

    chipNames.append(parallelChipDb.getNames());
    chipNames.append(spiChipDb.getNames());
    chipNames.append(parallelSerialChipDb.getNames());
    foreach (const QString &str, chipNames)
    {
        if (str.isEmpty())
            ui->chipSelectComboBox->addItem(QString("Unknown %1").arg(++i));
        else
            ui->chipSelectComboBox->addItem(str);
    }
    ui->chipSelectComboBox->setCurrentIndex(CHIP_INDEX_DEFAULT);
}

void MainWindow::slotAboutDialog()
{
    AboutDialog aboutDialog(this);

    aboutDialog.exec();
}

void MainWindow::setProgress(unsigned int progress)
{
    static unsigned int old_progress = 100;
    QTime Qtime_passed, Qtime_total;

    if(old_progress == progress)
        return;

    old_progress = progress;

    if(progress == 0)
    {
        timer.restart();
        Qtime_passed = QTime::fromMSecsSinceStartOfDay(0);
        Qtime_total = QTime::fromMSecsSinceStartOfDay(0);
    }
    else
    {
        Qtime_passed = QTime::fromMSecsSinceStartOfDay(timer.elapsed());
        Qtime_total = QTime::fromMSecsSinceStartOfDay(timer.elapsed() * 100 / progress);
    }
    statusBar()->showMessage(tr("Progress: %1%    Passed: %2    Total: %3")
                             .arg(progress)
                             .arg(Qtime_passed.toString("hh:mm:ss"))
                             .arg(Qtime_total.toString("hh:mm:ss")));

    if((progress == 100) && isAlertEnabled)
    {
        QMessageBox *msgBox = new QMessageBox(this);
        msgBox->setIcon(QMessageBox::Information);
        msgBox->setText("Completed.");
        msgBox->setAttribute(Qt::WA_DeleteOnClose);
        msgBox->open();
    }
}

void MainWindow::slotProgFirmwareUpdateCompleted(int status)
{
    disconnect(prog, SIGNAL(firmwareUpdateProgress(quint64)), this,
        SLOT(slotProgFirmwareUpdateProgress(quint64)));
    disconnect(prog, SIGNAL(firmwareUpdateCompleted(int)), this,
        SLOT(slotProgFirmwareUpdateCompleted(int)));

    if (!status)
        qInfo() << "Firmware update completed. Please restart device.";

    setProgress(100);
}

void MainWindow::slotProgFirmwareUpdateProgress(quint64 progress)
{
    setProgress(progress);
}

void MainWindow::slotFirmwareUpdateDialog()
{
    FirmwareUpdateDialog fwUpdateDialog(this);

    if (fwUpdateDialog.exec() != QDialog::Accepted)
        return;

    QString fileName = fwUpdateDialog.getFilePath();

    if (fileName.isNull())
        return;

    qInfo() << "Firmware update ...";
    connect(prog, SIGNAL(firmwareUpdateCompleted(int)), this,
        SLOT(slotProgFirmwareUpdateCompleted(int)));
    connect(prog, SIGNAL(firmwareUpdateProgress(quint64)), this,
        SLOT(slotProgFirmwareUpdateProgress(quint64)));
    prog->firmwareUpdate(fileName);
}

void MainWindow::slotSelectFilePath()
{
    QString filePath = ui->filePathLineEdit->text();

    QFileDialog selectFile(this);
    selectFile.setWindowTitle(tr("Choose a file."));
    selectFile.setDirectory(filePath);
    selectFile.setNameFilter(tr("Binary file(*.bin);;All Files(*)"));
    selectFile.setViewMode(QFileDialog::Detail);
    selectFile.setLabelText(QFileDialog::Accept, tr("Select"));
    selectFile.setLabelText(QFileDialog::Reject, tr("Cancel"));
    if (selectFile.exec())
    {
        filePath = selectFile.selectedFiles().at(0);
        ui->filePathLineEdit->setText(filePath);
        ui->dataViewer->setFile(filePath);
        QSettings settings(SETTINGS_ORGANIZATION_NAME, SETTINGS_APPLICATION_NAME);
        settings.setValue(SETTINGS_WORK_FILE_PATH, filePath);
    }
}

void MainWindow::slotFilePathEditingFinished()
{
    if (ui->filePathLineEdit->text().isEmpty())
        return;
    QString filePath = ui->filePathLineEdit->text();
    ui->dataViewer->setFile(filePath);
    QSettings settings(SETTINGS_ORGANIZATION_NAME, SETTINGS_APPLICATION_NAME);
    settings.setValue(SETTINGS_WORK_FILE_PATH, filePath);
}

