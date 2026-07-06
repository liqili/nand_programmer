/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#ifndef PARALLEL_SERIAL_CHIP_DB_TABLE_MODEL_H
#define PARALLEL_SERIAL_CHIP_DB_TABLE_MODEL_H

#include "parallel_serial_chip_db.h"
#include <QAbstractTableModel>

class ParallelSerialChipDbTableModel : public QAbstractTableModel
{
    Q_OBJECT

    ParallelSerialChipDb *chipDb;

public:
    explicit ParallelSerialChipDbTableModel(ParallelSerialChipDb *chipDb,
        QObject *parent = nullptr);
    int rowCount(const QModelIndex & /*parent*/) const override;
    int columnCount(const QModelIndex & /*parent*/) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role)
        const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
        int role = Qt::EditRole) override;
    void addRow();
    void delRow(int index);
    void commit();
    void reset();
};

#endif // PARALLEL_SERIAL_CHIP_DB_TABLE_MODEL_H
