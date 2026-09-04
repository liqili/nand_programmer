/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#ifndef ECC_SETTINGS_DIALOG_H
#define ECC_SETTINGS_DIALOG_H

#include "ecc/ecc_scheme.h"

#include <QDialog>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QToolButton;

/* Draws the spare area of a single sector byte by byte, so it is obvious at a
 * glance which bytes the codeword covers, which hold the parity, and which are
 * left free. Getting this layout wrong is the usual reason ECC appears to
 * "not work" against a real device, and a picture catches it faster than a
 * row of spin boxes does.
 */
class SpareMapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SpareMapWidget(QWidget *parent = nullptr);

    void setScheme(const EccScheme &scheme, bool valid);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    EccScheme m_scheme;
    bool m_valid;
};

class EccSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EccSettingsDialog(QWidget *parent = nullptr);

    /* Geometry of the currently selected chip, used to check that the scheme
     * fits and that parity does not land on the bad block marker. Pass 0, 0
     * when no chip is selected.
     */
    void setChipGeometry(int pageSize, int spareSize, int bbMarkOffset = -1);

    /* Whether the transfer carries the spare area. Parity lives there, so with
     * it switched off nothing configured here can take effect.
     */
    void setSpareIncluded(bool included);

    void setEccEnabled(bool enabled);
    bool isEccEnabled() const;

    void setScheme(const EccScheme &scheme);
    EccScheme scheme() const;

    void setVerifyOnRead(bool verify);
    bool isVerifyOnRead() const;

    void setCorrectOnRead(bool correct);
    bool isCorrectOnRead() const;

    void setGenerateOnWrite(bool generate);
    bool isGenerateOnWrite() const;

    void setStopOnUncorrectable(bool stop);
    bool isStopOnUncorrectable() const;

private slots:
    void slotPresetChanged(int index);
    void slotParamChanged();
    void slotAlgoChanged();
    void slotUsePolyDefault();
    void slotToggleAdvanced();
    void slotEnabledToggled(bool on);

private:
    void buildUi();
    void schemeToUi(const EccScheme &scheme);
    EccScheme uiToScheme() const;
    void refresh();
    uint32_t parsePoly(bool *ok) const;

    QCheckBox *m_enable;
    QComboBox *m_preset;
    QLabel *m_presetDesc;

    QRadioButton *m_readReport;
    QRadioButton *m_readCorrect;
    QCheckBox *m_stopOnUncorrectable;
    QCheckBox *m_generateOnWrite;

    QToolButton *m_advToggle;
    QGroupBox *m_advanced;
    QComboBox *m_algo;
    QComboBox *m_sectorSize;
    QSpinBox *m_oobSize;
    QSpinBox *m_m;
    QSpinBox *m_t;
    QLineEdit *m_poly;
    QPushButton *m_polyDefault;
    QLabel *m_polyNote;
    QSpinBox *m_eccBitOffset;
    QCheckBox *m_coverSpare;
    QSpinBox *m_truncate;

    QLabel *m_summary;
    QLabel *m_strength;
    QLabel *m_status;
    SpareMapWidget *m_map;

    QDialogButtonBox *m_buttons;

    QWidget *m_rowM;
    QWidget *m_rowT;
    QWidget *m_rowPoly;

    int m_pageSize;
    int m_spareSize;
    int m_bbMarkOffset;
    bool m_spareIncluded;
    bool m_updating;
};

#endif /* ECC_SETTINGS_DIALOG_H */
