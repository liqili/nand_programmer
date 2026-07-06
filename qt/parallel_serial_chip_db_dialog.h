/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#ifndef PARALLEL_SERIAL_CHIP_DB_DALOG_H
#define PARALLEL_SERIAL_CHIP_DB_DALOG_H

#include "parallel_serial_chip_db_table_model.h"
#include <QDialog>
#include <QSortFilterProxyModel>

namespace Ui {
class ParallelSerialChipDbDialog;
}

class ParallelSerialChipDbDialog : public QDialog
{
    Q_OBJECT

    Ui::ParallelSerialChipDbDialog *ui;
    ParallelSerialChipDbTableModel chipDbTableModel;
    QSortFilterProxyModel chipDbProxyModel;

public:
    explicit ParallelSerialChipDbDialog(ParallelSerialChipDb *chipDb,
        QWidget *parent = nullptr);
    ~ParallelSerialChipDbDialog();

private slots:
    void slotAddChipDbButtonClicked();
    void slotDelChipDbButtonClicked();
    void slotOkButtonClicked();
    void slotCancelButtonClicked();
};

#endif // PARALLEL_SERIAL_CHIP_DB_DALOG_H
