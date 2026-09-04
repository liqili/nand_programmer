/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "programmer.h"
#include "parallel_chip_db.h"
#include "spi_chip_db.h"
#include "parallel_serial_chip_db.h"
#include "ecc/ecc_stream.h"
#include <QMainWindow>
#include <QVector>
#include <QElapsedTimer>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    Programmer *prog;
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    SyncBuffer buffer;
    ChipId chipId;
    ParallelChipDb parallelChipDb;
    SpiChipDb spiChipDb;
    ParallelSerialChipDb parallelSerialChipDb;
    ChipDb *currentChipDb;
    QElapsedTimer timer;
    bool isAlertEnabled;
    QFile workFile;
    quint64 areaSize;
    uint32_t pageSize;

    /* Software ECC applied on this side of the USB link. The scheme is what
     * the user configured; the engine is that scheme bound to the geometry of
     * the chip currently selected.
     */
    EccScheme eccScheme;
    EccEngine eccEngine;
    EccPageStream eccStream;
    EccStats eccStats;
    bool eccCorrectOnRead;
    bool eccGenerateOnWrite;
    bool eccWarnUncorrectable;

    void initBufTable();
    void resetBufTable();
    void setUiStateConnected(bool isConnected);
    void setUiStateSelected(bool isSelected);
    void updateChipList();
    void setProgress(unsigned int progress);
    void updateProgSettings();
    void updateEccSettings();
    /* Rebind the engine to the selected chip. Returns false and reports why
     * when the configured scheme does not fit its geometry.
     */
    bool rebindEcc();
    /* How pages read from the chip should be treated, given the settings and
     * whether the engine is bound to this chip at all.
     */
    EccMode readEccMode() const;
    /* Move whatever the reader has produced into the work file, through the
     * ECC stream when one is active.
     */
    void flushReadBuffer(bool final);
    void reportEccStats();
    void detectChip(ChipDb *chipDb);
    void detectChipReadChipIdDelayed();
    void detectChipDelayed();
    void setChipNameDelayed();
private slots:
    void slotProgConnectCompleted(quint64 status);
    void slotProgReadDeviceIdCompleted(quint64 status);
    void slotProgReadCompleted(quint64 readBytes);
    void slotProgReadProgress(quint64 progress);
    void slotProgVerifyCompleted(quint64 readBytes);
    void slotProgVerifyProgress(quint64 progress);
    void slotProgWriteCompleted(int status);
    void slotProgWriteProgress(quint64 progress);
    void slotProgEraseCompleted(quint64 status);
    void slotProgEraseProgress(quint64 progress);
    void slotProgReadBadBlocksCompleted(quint64 status);
    void slotProgReadBadBlocksProgress(quint64 progress);
    void slotProgSelectCompleted(quint64 status);
    void slotProgDetectChipConfCompleted(quint64 status);
    void slotProgDetectChipReadChipIdCompleted(quint64 status);
    void slotProgFirmwareUpdateCompleted(int status);
    void slotProgFirmwareUpdateProgress(quint64 progress);
    void slotSelectFilePath();
    void slotFilePathEditingFinished();

public slots:
    void slotProgConnect();
    void slotProgReadDeviceId();
    void slotProgErase();
    void slotProgRead();
    void slotProgVerify();
    void slotProgWrite();
    void slotProgReadBadBlocks();
    void slotSelectChip(int selectedChipNum);
    void slotDetectChip();
    void slotSettingsProgrammer();
    void slotSettingsEcc();
    void slotCheckImage();
    void slotSettingsParallelChipDb();
    void slotSettingsSpiChipDb();
    void slotSettingsParallelSerialChipDb();
    void slotAboutDialog();
    void slotFirmwareUpdateDialog();
};

#endif // MAIN_WINDOW_H
