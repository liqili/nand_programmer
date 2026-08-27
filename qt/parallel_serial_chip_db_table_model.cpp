/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#include "parallel_serial_chip_db_table_model.h"
#include "parallel_serial_chip_info.h"
#include <limits>

ParallelSerialChipDbTableModel::ParallelSerialChipDbTableModel(
    ParallelSerialChipDb *chipDb, QObject *parent) : QAbstractTableModel(parent)
{
    this->chipDb = chipDb;
}

int ParallelSerialChipDbTableModel::rowCount(
    const QModelIndex & /*parent*/) const
{
    return chipDb->size();
}

int ParallelSerialChipDbTableModel::columnCount(
    const QModelIndex & /*parent*/) const
{
    return ParallelSerialChipDb::CHIP_PARAM_NUM;
}

QVariant ParallelSerialChipDbTableModel::data(const QModelIndex &index,
    int role) const
{
    int column;
    QString paramStr;

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    column = index.column();
    switch (column)
    {
    case ParallelSerialChipDb::CHIP_PARAM_NAME:
        return chipDb->getChipName(index.row());
    case ParallelSerialChipDb::CHIP_PARAM_PAGE_SIZE:
        chipDb->getHexStringFromParam(chipDb->getPageSize(index.row()),
            paramStr);
        return paramStr;
    case ParallelSerialChipDb::CHIP_PARAM_BLOCK_SIZE:
        chipDb->getHexStringFromParam(chipDb->getBlockSize(index.row()),
            paramStr);
        return paramStr;
    case ParallelSerialChipDb::CHIP_PARAM_TOTAL_SIZE:
        chipDb->getHexStringFromParam(chipDb->getTotalSize(index.row()),
            paramStr);
        return paramStr;
    case ParallelSerialChipDb::CHIP_PARAM_PAGE_OFF:
        return (uint)chipDb->getChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_PAGE_OFF);
    case ParallelSerialChipDb::CHIP_PARAM_ADDR_CYCLES:
        return (uint)chipDb->getChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_ADDR_CYCLES);
    case ParallelSerialChipDb::CHIP_PARAM_ID_ADDR_CYCLES:
        return (uint)chipDb->getChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_ID_ADDR_CYCLES);
    case ParallelSerialChipDb::CHIP_PARAM_READ_CMD:
        chipDb->getHexStringFromParam(chipDb->getChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_READ_CMD), paramStr);
        return paramStr;
    case ParallelSerialChipDb::CHIP_PARAM_READ_ID_CMD:
        chipDb->getHexStringFromParam(chipDb->getChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_READ_ID_CMD), paramStr);
        return paramStr;
    case ParallelSerialChipDb::CHIP_PARAM_WRITE_CMD:
        chipDb->getHexStringFromParam(chipDb->getChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_WRITE_CMD), paramStr);
        return paramStr;
    case ParallelSerialChipDb::CHIP_PARAM_WRITE_EN_CMD:
        chipDb->getHexStringFromOptParam(chipDb->getChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_WRITE_EN_CMD), paramStr);
        return paramStr;
    case ParallelSerialChipDb::CHIP_PARAM_ERASE_CMD:
        chipDb->getHexStringFromParam(chipDb->getChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_ERASE_CMD), paramStr);
        return paramStr;
    case ParallelSerialChipDb::CHIP_PARAM_STATUS_CMD:
        chipDb->getHexStringFromParam(chipDb->getChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_STATUS_CMD), paramStr);
        return paramStr;
    case ParallelSerialChipDb::CHIP_PARAM_BUSY_BIT:
        return (uint)chipDb->getChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_BUSY_BIT);
    case ParallelSerialChipDb::CHIP_PARAM_BUSY_STATE:
        return (uint)chipDb->getChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_BUSY_STATE);
    case ParallelSerialChipDb::CHIP_PARAM_SETUP_TIME:
        return (uint)chipDb->getChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_SETUP_TIME);
    case ParallelSerialChipDb::CHIP_PARAM_WAIT_SETUP_TIME:
        return (uint)chipDb->getChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_WAIT_SETUP_TIME);
    case ParallelSerialChipDb::CHIP_PARAM_HOLD_SETUP_TIME:
        return (uint)chipDb->getChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_HOLD_SETUP_TIME);
    case ParallelSerialChipDb::CHIP_PARAM_HI_Z_SETUP_TIME:
        return (uint)chipDb->getChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_HI_Z_SETUP_TIME);
    case ParallelSerialChipDb::CHIP_PARAM_CLR_SETUP_TIME:
        return (uint)chipDb->getChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_CLR_SETUP_TIME);
    case ParallelSerialChipDb::CHIP_PARAM_AR_SETUP_TIME:
        return (uint)chipDb->getChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_AR_SETUP_TIME);
    case ParallelSerialChipDb::CHIP_PARAM_ID1:
        chipDb->getHexStringFromParam(chipDb->getChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_ID1), paramStr);
        return paramStr;
    case ParallelSerialChipDb::CHIP_PARAM_ID2:
        chipDb->getHexStringFromParam(chipDb->getChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_ID2), paramStr);
        return paramStr;
    case ParallelSerialChipDb::CHIP_PARAM_ID3:
        chipDb->getHexStringFromOptParam(chipDb->getChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_ID3), paramStr);
        return paramStr;
    case ParallelSerialChipDb::CHIP_PARAM_ID4:
        chipDb->getHexStringFromOptParam(chipDb->getChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_ID4), paramStr);
        return paramStr;
    case ParallelSerialChipDb::CHIP_PARAM_ID5:
        chipDb->getHexStringFromOptParam(chipDb->getChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_ID5), paramStr);
        return paramStr;
    }

    return QVariant();
}

QVariant ParallelSerialChipDbTableModel::headerData(int section,
    Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        switch (section)
        {
        case ParallelSerialChipDb::CHIP_PARAM_NAME: return tr("Name");
        case ParallelSerialChipDb::CHIP_PARAM_PAGE_SIZE: return tr("Page size");
        case ParallelSerialChipDb::CHIP_PARAM_BLOCK_SIZE: return tr("Block size");
        case ParallelSerialChipDb::CHIP_PARAM_TOTAL_SIZE: return tr("Total size");
        case ParallelSerialChipDb::CHIP_PARAM_PAGE_OFF: return tr("Page off.");
        case ParallelSerialChipDb::CHIP_PARAM_ADDR_CYCLES:
            return tr("Addr. cycles");
        case ParallelSerialChipDb::CHIP_PARAM_ID_ADDR_CYCLES:
            return tr("ID addr. cycles");
        case ParallelSerialChipDb::CHIP_PARAM_READ_CMD: return tr("Read com.");
        case ParallelSerialChipDb::CHIP_PARAM_READ_ID_CMD: return tr("Read ID com.");
        case ParallelSerialChipDb::CHIP_PARAM_WRITE_CMD: return tr("Write com.");
        case ParallelSerialChipDb::CHIP_PARAM_WRITE_EN_CMD: return tr("Write en. com.");
        case ParallelSerialChipDb::CHIP_PARAM_ERASE_CMD: return tr("Erase com.");
        case ParallelSerialChipDb::CHIP_PARAM_STATUS_CMD: return tr("Status com.");
        case ParallelSerialChipDb::CHIP_PARAM_BUSY_BIT: return tr("Busy bit");
        case ParallelSerialChipDb::CHIP_PARAM_BUSY_STATE: return tr("Busy bit state");
        case ParallelSerialChipDb::CHIP_PARAM_SETUP_TIME: return tr("Setup time");
        case ParallelSerialChipDb::CHIP_PARAM_WAIT_SETUP_TIME: return tr("Wait setup time");
        case ParallelSerialChipDb::CHIP_PARAM_HOLD_SETUP_TIME: return tr("Hold setup time");
        case ParallelSerialChipDb::CHIP_PARAM_HI_Z_SETUP_TIME: return tr("Hi-Z setup time");
        case ParallelSerialChipDb::CHIP_PARAM_CLR_SETUP_TIME: return tr("CLR setup time");
        case ParallelSerialChipDb::CHIP_PARAM_AR_SETUP_TIME: return tr("AR setup time");
        case ParallelSerialChipDb::CHIP_PARAM_ID1: return tr("ID 1");
        case ParallelSerialChipDb::CHIP_PARAM_ID2: return tr("ID 2");
        case ParallelSerialChipDb::CHIP_PARAM_ID3: return tr("ID 3");
        case ParallelSerialChipDb::CHIP_PARAM_ID4: return tr("ID 4");
        case ParallelSerialChipDb::CHIP_PARAM_ID5: return tr("ID 5");
        }
    }

    if (role == Qt::ToolTipRole)
    {
        switch (section)
        {
        case ParallelSerialChipDb::CHIP_PARAM_NAME:
            return tr("Chip name");
        case ParallelSerialChipDb::CHIP_PARAM_PAGE_SIZE:
            return tr("Page size in bytes");
        case ParallelSerialChipDb::CHIP_PARAM_BLOCK_SIZE:
            return tr("Block size in bytes");
        case ParallelSerialChipDb::CHIP_PARAM_TOTAL_SIZE:
            return tr("Total size in bytes");
        case ParallelSerialChipDb::CHIP_PARAM_PAGE_OFF:
            return tr("Page offset in address");
        case ParallelSerialChipDb::CHIP_PARAM_ADDR_CYCLES:
            return tr("Number of address cycles per transfer (1-4)");
        case ParallelSerialChipDb::CHIP_PARAM_ID_ADDR_CYCLES:
            return tr("Address cycles for read ID command: 0 for JEDEC 9Fh, 1 for 90h");
        case ParallelSerialChipDb::CHIP_PARAM_READ_CMD:
            return tr("Page read command");
        case ParallelSerialChipDb::CHIP_PARAM_READ_ID_CMD:
            return tr("Read ID command");
        case ParallelSerialChipDb::CHIP_PARAM_WRITE_CMD:
            return tr("Page write command");
        case ParallelSerialChipDb::CHIP_PARAM_WRITE_EN_CMD:
            return tr("Write enable command");
        case ParallelSerialChipDb::CHIP_PARAM_ERASE_CMD:
            return tr("Block erase command");
        case ParallelSerialChipDb::CHIP_PARAM_STATUS_CMD:
            return tr("Read status command");
        case ParallelSerialChipDb::CHIP_PARAM_BUSY_BIT:
            return tr("Busy bit number (0-7) in status register");
        case ParallelSerialChipDb::CHIP_PARAM_BUSY_STATE:
            return tr("Busy bit active state (0/1)");
        case ParallelSerialChipDb::CHIP_PARAM_SETUP_TIME:
            return tr("Setup time in ns");
        case ParallelSerialChipDb::CHIP_PARAM_WAIT_SETUP_TIME:
            return tr("Wait setup time in ns");
        case ParallelSerialChipDb::CHIP_PARAM_HOLD_SETUP_TIME:
            return tr("Hold setup time in ns");
        case ParallelSerialChipDb::CHIP_PARAM_HI_Z_SETUP_TIME:
            return tr("Hi-Z setup time in ns");
        case ParallelSerialChipDb::CHIP_PARAM_CLR_SETUP_TIME:
            return tr("CLR setup time in ns");
        case ParallelSerialChipDb::CHIP_PARAM_AR_SETUP_TIME:
            return tr("AR setup time in ns");
        case ParallelSerialChipDb::CHIP_PARAM_ID1:
            return tr("Chip ID 1st byte");
        case ParallelSerialChipDb::CHIP_PARAM_ID2:
            return tr("Chip ID 2nd byte");
        case ParallelSerialChipDb::CHIP_PARAM_ID3:
            return tr("Chip ID 3rd byte");
        case ParallelSerialChipDb::CHIP_PARAM_ID4:
            return tr("Chip ID 4th byte");
        case ParallelSerialChipDb::CHIP_PARAM_ID5:
            return tr("Chip ID 5th byte");
        }
    }

    return QVariant();
}

Qt::ItemFlags ParallelSerialChipDbTableModel::flags(
    const QModelIndex &index) const
{
    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

bool ParallelSerialChipDbTableModel::setData(const QModelIndex &index,
    const QVariant &value, int role)
{
    quint64 paramVal;

    if (role != Qt::EditRole)
        return false;

    switch (index.column())
    {
    case ParallelSerialChipDb::CHIP_PARAM_NAME:
        chipDb->setChipName(index.row(), value.toString());
        return true;
    case ParallelSerialChipDb::CHIP_PARAM_PAGE_SIZE:
        if (chipDb->getParamFromHexString(value.toString(), paramVal))
            return false;
        chipDb->setPageSize(index.row(), paramVal);
        return true;
    case ParallelSerialChipDb::CHIP_PARAM_BLOCK_SIZE:
        if (chipDb->getParamFromHexString(value.toString(), paramVal))
            return false;
        chipDb->setBlockSize(index.row(), paramVal);
        return true;
    case ParallelSerialChipDb::CHIP_PARAM_TOTAL_SIZE:
        if (chipDb->getParamFromHexString(value.toString(), paramVal))
            return false;
        chipDb->setTotalSize(index.row(), paramVal);
        return true;
    case ParallelSerialChipDb::CHIP_PARAM_PAGE_OFF:
        if (chipDb->getParamFromString(value.toString(), paramVal))
            return false;
        if (!chipDb->isParamValid(paramVal, 0, 0xFF))
            return false;
        chipDb->setChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_PAGE_OFF, paramVal);
        return true;
    case ParallelSerialChipDb::CHIP_PARAM_ADDR_CYCLES:
        if (chipDb->getParamFromString(value.toString(), paramVal))
            return false;
        if (!chipDb->isParamValid(paramVal, 1, 4))
            return false;
        chipDb->setChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_ADDR_CYCLES, paramVal);
        return true;
    case ParallelSerialChipDb::CHIP_PARAM_ID_ADDR_CYCLES:
        if (chipDb->getParamFromString(value.toString(), paramVal))
            return false;
        if (!chipDb->isParamValid(paramVal, 0, 4))
            return false;
        chipDb->setChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_ID_ADDR_CYCLES, paramVal);
        return true;
    case ParallelSerialChipDb::CHIP_PARAM_READ_CMD:
        if (chipDb->getParamFromHexString(value.toString(), paramVal))
            return false;
        if (!chipDb->isParamValid(paramVal, 0x00, 0xFF))
            return false;
        chipDb->setChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_READ_CMD, paramVal);
        return true;
    case ParallelSerialChipDb::CHIP_PARAM_READ_ID_CMD:
        if (chipDb->getParamFromHexString(value.toString(), paramVal))
            return false;
        if (!chipDb->isParamValid(paramVal, 0x00, 0xFF))
            return false;
        chipDb->setChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_READ_ID_CMD, paramVal);
        return true;
    case ParallelSerialChipDb::CHIP_PARAM_WRITE_CMD:
        if (chipDb->getParamFromHexString(value.toString(), paramVal))
            return false;
        if (!chipDb->isParamValid(paramVal, 0x00, 0xFF))
            return false;
        chipDb->setChipParam(index.row(),
            ParallelSerialChipInfo::CHIP_PARAM_WRITE_CMD, paramVal);
        return true;
    case ParallelSerialChipDb::CHIP_PARAM_WRITE_EN_CMD:
	        if (chipDb->getOptParamFromHexString(value.toString(), paramVal))
	            return false;
	        if (!chipDb->isOptParamValid(paramVal, 0x00, 0xFF))
	            return false;
	        chipDb->setChipParam(index.row(),
	            ParallelSerialChipInfo::CHIP_PARAM_WRITE_EN_CMD, paramVal);
	        return true;
	    case ParallelSerialChipDb::CHIP_PARAM_ERASE_CMD:
	        if (chipDb->getParamFromHexString(value.toString(), paramVal))
	            return false;
	        if (!chipDb->isParamValid(paramVal, 0x00, 0xFF))
	            return false;
	        chipDb->setChipParam(index.row(),
	            ParallelSerialChipInfo::CHIP_PARAM_ERASE_CMD, paramVal);
	        return true;
	    case ParallelSerialChipDb::CHIP_PARAM_STATUS_CMD:
	        if (chipDb->getParamFromHexString(value.toString(), paramVal))
	            return false;
	        if (!chipDb->isParamValid(paramVal, 0x00, 0xFF))
	            return false;
	        chipDb->setChipParam(index.row(),
	            ParallelSerialChipInfo::CHIP_PARAM_STATUS_CMD, paramVal);
	        return true;
	    case ParallelSerialChipDb::CHIP_PARAM_BUSY_BIT:
	        if (chipDb->getParamFromString(value.toString(), paramVal))
	            return false;
	        if (!chipDb->isParamValid(paramVal, 0, 7))
	            return false;
	        chipDb->setChipParam(index.row(),
	            ParallelSerialChipInfo::CHIP_PARAM_BUSY_BIT, paramVal);
	        return true;
	    case ParallelSerialChipDb::CHIP_PARAM_BUSY_STATE:
	        if (chipDb->getParamFromString(value.toString(), paramVal))
	            return false;
	        if (!chipDb->isParamValid(paramVal, 0, 1))
	            return false;
	        chipDb->setChipParam(index.row(),
	            ParallelSerialChipInfo::CHIP_PARAM_BUSY_STATE, paramVal);
	        return true;
	    case ParallelSerialChipDb::CHIP_PARAM_SETUP_TIME:
	        if (chipDb->getParamFromString(value.toString(), paramVal))
	            return false;
	        chipDb->setChipParam(index.row(),
	            ParallelSerialChipInfo::CHIP_PARAM_SETUP_TIME, paramVal);
	        return true;
	    case ParallelSerialChipDb::CHIP_PARAM_WAIT_SETUP_TIME:
	        if (chipDb->getParamFromString(value.toString(), paramVal))
	            return false;
	        chipDb->setChipParam(index.row(),
	            ParallelSerialChipInfo::CHIP_PARAM_WAIT_SETUP_TIME, paramVal);
	        return true;
	    case ParallelSerialChipDb::CHIP_PARAM_HOLD_SETUP_TIME:
	        if (chipDb->getParamFromString(value.toString(), paramVal))
	            return false;
	        chipDb->setChipParam(index.row(),
	            ParallelSerialChipInfo::CHIP_PARAM_HOLD_SETUP_TIME, paramVal);
	        return true;
	    case ParallelSerialChipDb::CHIP_PARAM_HI_Z_SETUP_TIME:
	        if (chipDb->getParamFromString(value.toString(), paramVal))
	            return false;
	        chipDb->setChipParam(index.row(),
	            ParallelSerialChipInfo::CHIP_PARAM_HI_Z_SETUP_TIME, paramVal);
	        return true;
	    case ParallelSerialChipDb::CHIP_PARAM_CLR_SETUP_TIME:
	        if (chipDb->getParamFromString(value.toString(), paramVal))
	            return false;
	        chipDb->setChipParam(index.row(),
	            ParallelSerialChipInfo::CHIP_PARAM_CLR_SETUP_TIME, paramVal);
	        return true;
	    case ParallelSerialChipDb::CHIP_PARAM_AR_SETUP_TIME:
	        if (chipDb->getParamFromString(value.toString(), paramVal))
	            return false;
	        chipDb->setChipParam(index.row(),
	            ParallelSerialChipInfo::CHIP_PARAM_AR_SETUP_TIME, paramVal);
	        return true;
	    case ParallelSerialChipDb::CHIP_PARAM_ID1:
	        if (chipDb->getParamFromHexString(value.toString(), paramVal))
	            return false;
	        if (!chipDb->isParamValid(paramVal, 0x00, 0xFF))
	            return false;
	        chipDb->setChipParam(index.row(),
	            ParallelSerialChipInfo::CHIP_PARAM_ID1, paramVal);
	        return true;
	    case ParallelSerialChipDb::CHIP_PARAM_ID2:
	        if (chipDb->getParamFromHexString(value.toString(), paramVal))
	            return false;
	        if (!chipDb->isParamValid(paramVal, 0x00, 0xFF))
	            return false;
	        chipDb->setChipParam(index.row(),
	            ParallelSerialChipInfo::CHIP_PARAM_ID2, paramVal);
	        return true;
	    case ParallelSerialChipDb::CHIP_PARAM_ID3:
	        if (chipDb->getOptParamFromHexString(value.toString(), paramVal))
	            return false;
	        if (!chipDb->isOptParamValid(paramVal, 0x00, 0xFF))
	            return false;
	        chipDb->setChipParam(index.row(),
	            ParallelSerialChipInfo::CHIP_PARAM_ID3, paramVal);
	        return true;
	    case ParallelSerialChipDb::CHIP_PARAM_ID4:
	        if (chipDb->getOptParamFromHexString(value.toString(), paramVal))
	            return false;
	        if (!chipDb->isOptParamValid(paramVal, 0x00, 0xFF))
	            return false;
	        chipDb->setChipParam(index.row(),
	            ParallelSerialChipInfo::CHIP_PARAM_ID4, paramVal);
	        return true;
	    case ParallelSerialChipDb::CHIP_PARAM_ID5:
	        if (chipDb->getOptParamFromHexString(value.toString(), paramVal))
	            return false;
	        if (!chipDb->isOptParamValid(paramVal, 0x00, 0xFF))
	            return false;
	        chipDb->setChipParam(index.row(),
	            ParallelSerialChipInfo::CHIP_PARAM_ID5, paramVal);
	        return true;
	    }

	    return false;
	}

	void ParallelSerialChipDbTableModel::addRow()
	{
	    ParallelSerialChipInfo *chipInfo = new ParallelSerialChipInfo();

	    beginResetModel();
	    chipDb->addChip(chipInfo);
	    endResetModel();
	}

	void ParallelSerialChipDbTableModel::delRow(int index)
	{
	    beginResetModel();
	    chipDb->delChip(index);
	    endResetModel();
	}

	void ParallelSerialChipDbTableModel::commit()
	{
	    chipDb->commit();
	}

	void ParallelSerialChipDbTableModel::reset()
	{
	    beginResetModel();
	    chipDb->reset();
	    endResetModel();
	}
