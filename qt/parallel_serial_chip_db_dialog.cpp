/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#include "parallel_serial_chip_db_dialog.h"
#include "ui_parallel_serial_chip_db_dialog.h"

#define HEADER_LONG_WIDTH 120
#define HEADER_MED_WIDTH 110
#define HEADER_SHORT_WIDTH 50

ParallelSerialChipDbDialog::ParallelSerialChipDbDialog(ParallelSerialChipDb *chipDb,
    QWidget *parent) : QDialog(parent), ui(new Ui::ParallelSerialChipDbDialog),
    chipDbTableModel(chipDb, parent)
{
    ui->setupUi(this);

#ifdef Q_OS_WIN32
    QFont font("Courier New", 6);
    ui->chipDbTableView->setFont(font);
#endif

    chipDbProxyModel.setSourceModel(&chipDbTableModel);
    ui->chipDbTableView->setModel(&chipDbProxyModel);
    ui->chipDbTableView->setColumnWidth(ParallelSerialChipDb::CHIP_PARAM_NAME,
        HEADER_LONG_WIDTH);
    ui->chipDbTableView->setColumnWidth(ParallelSerialChipDb::CHIP_PARAM_PAGE_SIZE,
        HEADER_MED_WIDTH);
    ui->chipDbTableView->setColumnWidth(ParallelSerialChipDb::CHIP_PARAM_BLOCK_SIZE,
        HEADER_MED_WIDTH);
    ui->chipDbTableView->setColumnWidth(ParallelSerialChipDb::CHIP_PARAM_TOTAL_SIZE,
        HEADER_MED_WIDTH);

    for (int i = ParallelSerialChipDb::CHIP_PARAM_PAGE_OFF;
         i <= ParallelSerialChipDb::CHIP_PARAM_AR_SETUP_TIME; i++)
    {
        ui->chipDbTableView->setColumnWidth(i, HEADER_MED_WIDTH);
    }

    connect(ui->addChipDbButton, SIGNAL(clicked()), this,
        SLOT(slotAddChipDbButtonClicked()));
    connect(ui->delChipDbButton, SIGNAL(clicked()), this,
        SLOT(slotDelChipDbButtonClicked()));
    connect(ui->okCancelButtonBox->button(QDialogButtonBox::Ok),
        SIGNAL(clicked()), this, SLOT(slotOkButtonClicked()));
    connect(ui->okCancelButtonBox->button(QDialogButtonBox::Cancel),
        SIGNAL(clicked()), this, SLOT(slotCancelButtonClicked()));
}

ParallelSerialChipDbDialog::~ParallelSerialChipDbDialog()
{
    delete ui;
}

void ParallelSerialChipDbDialog::slotAddChipDbButtonClicked()
{
    chipDbTableModel.addRow();
}

void ParallelSerialChipDbDialog::slotDelChipDbButtonClicked()
{
    QModelIndexList selection = ui->chipDbTableView->selectionModel()->
        selectedRows();

    if (!selection.count())
        return;

    chipDbTableModel.delRow(selection.at(0).row());
}

void ParallelSerialChipDbDialog::slotOkButtonClicked()
{
    chipDbTableModel.commit();
}

void ParallelSerialChipDbDialog::slotCancelButtonClicked()
{
    chipDbTableModel.reset();
}
