/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#include "ecc_settings_dialog.h"
#include "ecc/ecc_engine.h"
#include "ecc/gf.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFontMetrics>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>
#include <QRegExpValidator>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>

namespace
{

/* Palette for the spare area map. Chosen to stay legible on both the light and
 * dark platform themes rather than relying on the window background.
 */
const QColor COLOR_COVERED(0x4C, 0x8B, 0xF5);   /* protected by the codeword */
const QColor COLOR_PARITY(0xE8, 0x8B, 0x2E);    /* holds the parity          */
const QColor COLOR_FREE(0x9E, 0x9E, 0x9E);      /* left for the user         */
const QColor COLOR_INVALID(0xC0, 0x39, 0x2B);

/* A wrapped QLabel reports a height that ignores its width unless asked to do
 * otherwise, which lets a layout squeeze it until the text is clipped.
 */
void makeWrapping(QLabel *label)
{
    QSizePolicy sp = label->sizePolicy();

    sp.setHeightForWidth(true);
    sp.setVerticalPolicy(QSizePolicy::MinimumExpanding);
    label->setSizePolicy(sp);
    label->setWordWrap(true);
    label->setMinimumHeight(label->fontMetrics().height() + 2);
}

QWidget *wrap(QWidget *inner, const QString &suffix = QString())
{
    QWidget *w = new QWidget;
    QHBoxLayout *l = new QHBoxLayout(w);

    l->setContentsMargins(0, 0, 0, 0);
    l->addWidget(inner);

    if (!suffix.isEmpty())
        l->addWidget(new QLabel(suffix));

    l->addStretch();

    return w;
}

} /* namespace */

/* ------------------------------------------------------------------ map -- */

SpareMapWidget::SpareMapWidget(QWidget *parent) : QWidget(parent),
    m_valid(true)
{
    /* Fixed vertically: the drawing has a fixed number of rows, and letting a
     * layout compress it makes the legend collide with whatever follows.
     */
    setMinimumHeight(80);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
}

void SpareMapWidget::setScheme(const EccScheme &scheme, bool valid)
{
    m_scheme = scheme;
    m_valid = valid;
    update();
}

QSize SpareMapWidget::sizeHint() const
{
    return QSize(420, 80);
}

void SpareMapWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    p.setRenderHint(QPainter::Antialiasing, false);

    const int n = m_scheme.oobSize;
    if (n <= 0 || m_scheme.algo == ECC_ALGO_NONE)
    {
        p.setPen(palette().color(QPalette::Disabled, QPalette::WindowText));
        p.drawText(rect(), Qt::AlignCenter,
            m_scheme.algo == ECC_ALGO_NONE ?
            tr("No parity is stored in the spare area.") :
            tr("Nothing to show."));
        return;
    }

    const int top = 4;
    const int h = 22;
    const int w = width() - 2;
    const double cell = static_cast<double>(w) / n;
    const int eccStart = m_scheme.eccBitOffset;
    const int eccEnd = eccStart + m_scheme.eccBits();

    for (int b = 0; b < n; b++)
    {
        const double x0 = 1 + b * cell;
        const int bitLo = b * 8;

        /* Fill the byte in bit sized slices so a parity region that starts
         * half way through a byte is drawn where it really starts.
         */
        for (int bit = 0; bit < 8; bit++)
        {
            const int abs = bitLo + bit;
            QColor c;

            if (!m_valid)
                c = COLOR_INVALID;
            else if (abs >= eccStart && abs < eccEnd)
                c = COLOR_PARITY;
            else if (m_scheme.coverSpare && abs < eccStart)
                c = COLOR_COVERED;
            else
                c = COLOR_FREE;

            const double sx = x0 + cell * bit / 8.0;
            p.fillRect(QRectF(sx, top, cell / 8.0 + 0.5, h), c);
        }

        p.setPen(palette().color(QPalette::Mid));
        p.drawRect(QRectF(x0, top, cell, h));
    }

    /* Byte indices underneath, thinned out when the cells get narrow. */
    QFont f = font();
    f.setPointSizeF(f.pointSizeF() - 1.5);
    p.setFont(f);
    p.setPen(palette().color(QPalette::WindowText));

    const int step = cell < 18 ? (cell < 10 ? 4 : 2) : 1;
    for (int b = 0; b < n; b += step)
    {
        /* Centre the number on its cell but give it a wider box, otherwise a
         * two digit index is clipped once the cells get narrow.
         */
        p.drawText(QRectF(1 + b * cell - cell, top + h + 2, cell * 3, 14),
            Qt::AlignCenter, QString::number(b));
    }

    /* Legend. */
    const int ly = top + h + 22;
    int lx = 1;
    struct { QColor c; QString t; bool on; } items[] =
    {
        { COLOR_COVERED, tr("covered"), m_scheme.coverSpare },
        { COLOR_PARITY, tr("parity"), true },
        { COLOR_FREE, tr("free"), true },
    };

    for (int i = 0; i < 3; i++)
    {
        if (!items[i].on)
            continue;

        p.fillRect(QRect(lx, ly + 2, 9, 9), items[i].c);
        p.setPen(palette().color(QPalette::WindowText));
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        const int tw = QFontMetrics(f).horizontalAdvance(items[i].t);
#else
        const int tw = QFontMetrics(f).width(items[i].t);
#endif
        p.drawText(QRect(lx + 13, ly, tw + 4, 14), Qt::AlignVCenter | Qt::AlignLeft,
            items[i].t);
        lx += 13 + tw + 14;
    }
}

/* --------------------------------------------------------------- dialog -- */

EccSettingsDialog::EccSettingsDialog(QWidget *parent) : QDialog(parent),
    m_pageSize(0), m_spareSize(0), m_bbMarkOffset(-1), m_spareIncluded(true),
    m_updating(false)
{
    buildUi();
    schemeToUi(EccScheme::preset(0));
    refresh();
}

void EccSettingsDialog::buildUi()
{
    setWindowTitle(tr("ECC Settings"));

    QVBoxLayout *root = new QVBoxLayout(this);

    /* -- master switch -- */
    m_enable = new QCheckBox(tr("Apply error correction"));
    m_enable->setToolTip(tr("When off, pages are read and written exactly as "
        "they are stored, with no parity generated or checked."));
    root->addWidget(m_enable);

    /* -- preset -- */
    QFormLayout *presetForm = new QFormLayout;
    m_preset = new QComboBox;
    for (int i = 0; i < EccScheme::presetCount(); i++)
        m_preset->addItem(QString::fromUtf8(EccScheme::presetName(i)));
    m_preset->addItem(tr("Custom"));
    presetForm->addRow(tr("Layout:"), m_preset);
    root->addLayout(presetForm);

    m_presetDesc = new QLabel;
    m_presetDesc->setEnabled(false);
    makeWrapping(m_presetDesc);
    m_presetDesc->setMinimumHeight(m_presetDesc->fontMetrics().height() * 3);
    root->addWidget(m_presetDesc);

    /* -- behaviour -- */
    QGroupBox *readBox = new QGroupBox(tr("When reading"));
    QVBoxLayout *readLay = new QVBoxLayout(readBox);
    m_readReport = new QRadioButton(tr("Report errors only, leave data as read"));
    m_readCorrect = new QRadioButton(tr("Correct errors in the data"));
    m_readCorrect->setChecked(true);
    m_stopOnUncorrectable = new QCheckBox(
        tr("Warn when a page cannot be corrected"));
    m_stopOnUncorrectable->setChecked(true);
    readLay->addWidget(m_readReport);
    readLay->addWidget(m_readCorrect);
    readLay->addWidget(m_stopOnUncorrectable);
    root->addWidget(readBox);

    QGroupBox *writeBox = new QGroupBox(tr("When writing"));
    QVBoxLayout *writeLay = new QVBoxLayout(writeBox);
    m_generateOnWrite = new QCheckBox(
        tr("Generate parity into the spare area"));
    m_generateOnWrite->setChecked(true);
    m_generateOnWrite->setToolTip(tr("Parity is computed from the data being "
        "written and placed in the spare area at the offset below."));
    writeLay->addWidget(m_generateOnWrite);
    root->addWidget(writeBox);

    /* -- advanced -- */
    m_advToggle = new QToolButton;
    m_advToggle->setText(tr("Advanced parameters"));
    m_advToggle->setCheckable(true);
    m_advToggle->setChecked(false);
    m_advToggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_advToggle->setArrowType(Qt::RightArrow);
    m_advToggle->setAutoRaise(true);
    root->addWidget(m_advToggle);

    m_advanced = new QGroupBox;
    m_advanced->setVisible(false);
    QFormLayout *adv = new QFormLayout(m_advanced);

    m_algo = new QComboBox;
    m_algo->addItem(tr("None"), ECC_ALGO_NONE);
    m_algo->addItem(tr("Hamming"), ECC_ALGO_HAMMING);
    m_algo->addItem(tr("BCH"), ECC_ALGO_BCH);
    m_algo->addItem(tr("Reed-Solomon"), ECC_ALGO_RS);
    adv->addRow(tr("Algorithm:"), m_algo);

    m_sectorSize = new QComboBox;
    m_sectorSize->addItem(tr("256 bytes"), 256);
    m_sectorSize->addItem(tr("512 bytes"), 512);
    m_sectorSize->addItem(tr("1024 bytes"), 1024);
    adv->addRow(tr("Sector size:"), m_sectorSize);

    m_oobSize = new QSpinBox;
    m_oobSize->setRange(1, 512);
    adv->addRow(tr("Spare per sector:"), wrap(m_oobSize, tr("bytes")));

    m_m = new QSpinBox;
    m_m->setRange(3, 15);
    m_m->setToolTip(tr("Galois field order. The codeword cannot exceed "
        "2^m - 1 bits, so larger sectors need a larger m."));
    m_rowM = wrap(m_m);
    adv->addRow(tr("Field order m:"), m_rowM);

    m_t = new QSpinBox;
    m_t->setRange(1, 64);
    m_t->setToolTip(tr("Correction strength. For BCH this is bits per sector; "
        "for Reed-Solomon it is symbols."));
    m_rowT = wrap(m_t);
    adv->addRow(tr("Strength t:"), m_rowT);

    m_poly = new QLineEdit;
    m_poly->setPlaceholderText(tr("default for m"));
    m_poly->setValidator(new QRegExpValidator(
        QRegExp("0?[xX]?[0-9a-fA-F]{0,8}"), this));
    m_poly->setToolTip(tr("Primitive polynomial as a bit mask including the "
        "x^m term, for example 0x201b for x^13 + x^4 + x^3 + x + 1. "
        "Leave empty to use the conventional polynomial for m."));
    m_polyDefault = new QPushButton(tr("Default"));
    m_polyDefault->setAutoDefault(false);

    QWidget *polyRow = new QWidget;
    QHBoxLayout *polyLay = new QHBoxLayout(polyRow);
    polyLay->setContentsMargins(0, 0, 0, 0);
    polyLay->addWidget(m_poly, 1);
    polyLay->addWidget(m_polyDefault);
    m_rowPoly = polyRow;
    adv->addRow(tr("Primitive polynomial:"), m_rowPoly);

    m_polyNote = new QLabel;
    m_polyNote->setEnabled(false);
    adv->addRow(QString(), m_polyNote);

    m_eccBitOffset = new QSpinBox;
    m_eccBitOffset->setRange(0, 4096);
    m_eccBitOffset->setToolTip(tr("Where the parity starts, counted in bits "
        "from the first spare byte of the sector. Broadcom BCH-4 starts at "
        "bit 76, half way through byte 9."));
    adv->addRow(tr("Parity offset:"),
        wrap(m_eccBitOffset, tr("bits into spare")));

    m_coverSpare = new QCheckBox(
        tr("Protect the spare bytes before the parity"));
    m_coverSpare->setToolTip(tr("When set, the bytes of the spare area that "
        "precede the parity are part of the codeword, so user metadata is "
        "protected too."));
    adv->addRow(QString(), m_coverSpare);

    m_truncate = new QSpinBox;
    m_truncate->setRange(0, 32);
    m_truncate->setToolTip(tr("Number of high parity bits an encoder failed "
        "to store. Set this only to match images produced by a tool with that "
        "behaviour; real controllers store every bit."));
    adv->addRow(tr("Truncated top bits:"), wrap(m_truncate, tr("bits")));

    root->addWidget(m_advanced);

    /* -- live result -- */
    QGroupBox *resBox = new QGroupBox(tr("Result"));
    QVBoxLayout *resLay = new QVBoxLayout(resBox);

    m_summary = new QLabel;
    makeWrapping(m_summary);
    resLay->addWidget(m_summary);

    m_strength = new QLabel;
    makeWrapping(m_strength);
    resLay->addWidget(m_strength);

    m_map = new SpareMapWidget;
    resLay->addWidget(m_map);

    m_status = new QLabel;
    makeWrapping(m_status);
    resLay->addWidget(m_status);

    root->addWidget(resBox);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok |
        QDialogButtonBox::Cancel);
    root->addWidget(m_buttons);

    connect(m_buttons, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttons, SIGNAL(rejected()), this, SLOT(reject()));

    connect(m_enable, SIGNAL(toggled(bool)), this,
        SLOT(slotEnabledToggled(bool)));
    connect(m_preset, SIGNAL(currentIndexChanged(int)), this,
        SLOT(slotPresetChanged(int)));
    connect(m_advToggle, SIGNAL(clicked()), this, SLOT(slotToggleAdvanced()));
    connect(m_polyDefault, SIGNAL(clicked()), this, SLOT(slotUsePolyDefault()));
    connect(m_algo, SIGNAL(currentIndexChanged(int)), this,
        SLOT(slotAlgoChanged()));

    connect(m_sectorSize, SIGNAL(currentIndexChanged(int)), this,
        SLOT(slotParamChanged()));
    connect(m_oobSize, SIGNAL(valueChanged(int)), this,
        SLOT(slotParamChanged()));
    connect(m_m, SIGNAL(valueChanged(int)), this, SLOT(slotParamChanged()));
    connect(m_t, SIGNAL(valueChanged(int)), this, SLOT(slotParamChanged()));
    connect(m_poly, SIGNAL(textChanged(QString)), this,
        SLOT(slotParamChanged()));
    connect(m_eccBitOffset, SIGNAL(valueChanged(int)), this,
        SLOT(slotParamChanged()));
    connect(m_coverSpare, SIGNAL(toggled(bool)), this,
        SLOT(slotParamChanged()));
    connect(m_truncate, SIGNAL(valueChanged(int)), this,
        SLOT(slotParamChanged()));
}

void EccSettingsDialog::setSpareIncluded(bool included)
{
    m_spareIncluded = included;
    refresh();
}

void EccSettingsDialog::setChipGeometry(int pageSize, int spareSize,
    int bbMarkOffset)
{
    m_pageSize = pageSize;
    m_spareSize = spareSize;
    m_bbMarkOffset = bbMarkOffset;
    refresh();
}

void EccSettingsDialog::slotToggleAdvanced()
{
    const bool on = m_advToggle->isChecked();

    m_advanced->setVisible(on);
    m_advToggle->setArrowType(on ? Qt::DownArrow : Qt::RightArrow);
    adjustSize();
}

void EccSettingsDialog::slotEnabledToggled(bool on)
{
    refresh();
    (void)on;
}

void EccSettingsDialog::slotPresetChanged(int index)
{
    if (m_updating)
        return;

    if (index >= 0 && index < EccScheme::presetCount())
    {
        schemeToUi(EccScheme::preset(index));
        refresh();
    }
}

void EccSettingsDialog::slotAlgoChanged()
{
    if (m_updating)
        return;

    /* Moving to a different algorithm makes the previous field parameters
     * meaningless, so seed sensible ones rather than leaving stale values.
     */
    const EccAlgorithm a =
        static_cast<EccAlgorithm>(m_algo->currentData().toInt());

    m_updating = true;
    if (a == ECC_ALGO_BCH && m_m->value() < 13)
        m_m->setValue(13);
    else if (a == ECC_ALGO_RS && m_m->value() != 10)
        m_m->setValue(10);
    m_updating = false;

    slotParamChanged();
}

void EccSettingsDialog::slotUsePolyDefault()
{
    m_poly->clear();
}

void EccSettingsDialog::slotParamChanged()
{
    if (m_updating)
        return;

    /* Any hand edit means the layout no longer is one of the named ones,
     * unless it happens to land exactly on another preset.
     */
    const EccScheme s = uiToScheme();
    const int match = s.matchingPreset();

    m_updating = true;
    m_preset->setCurrentIndex(match >= 0 ? match :
        EccScheme::presetCount());
    m_updating = false;

    refresh();
}

uint32_t EccSettingsDialog::parsePoly(bool *ok) const
{
    QString t = m_poly->text().trimmed();

    if (ok)
        *ok = true;

    if (t.isEmpty())
        return 0;

    if (t.startsWith("0x", Qt::CaseInsensitive))
        t = t.mid(2);
    else if (t.startsWith("x", Qt::CaseInsensitive))
        t = t.mid(1);

    if (t.isEmpty())
        return 0;

    bool conv = false;
    const uint32_t v = t.toUInt(&conv, 16);

    if (ok)
        *ok = conv;

    return conv ? v : 0;
}

void EccSettingsDialog::schemeToUi(const EccScheme &scheme)
{
    m_updating = true;

    const int algoIdx = m_algo->findData(scheme.algo);
    m_algo->setCurrentIndex(algoIdx >= 0 ? algoIdx : 0);

    const int ssIdx = m_sectorSize->findData(scheme.sectorSize);
    m_sectorSize->setCurrentIndex(ssIdx >= 0 ? ssIdx : 1);

    m_oobSize->setValue(scheme.oobSize);
    m_m->setValue(scheme.m >= 3 ? scheme.m : 13);
    m_t->setValue(scheme.t >= 1 ? scheme.t : 4);

    if (scheme.primPoly)
        m_poly->setText(QString("0x%1").arg(scheme.primPoly, 0, 16));
    else
        m_poly->clear();

    m_eccBitOffset->setValue(scheme.eccBitOffset);
    m_coverSpare->setChecked(scheme.coverSpare);
    m_truncate->setValue(scheme.truncateTopBits);

    const int match = scheme.matchingPreset();
    m_preset->setCurrentIndex(match >= 0 ? match : EccScheme::presetCount());

    m_updating = false;
}

EccScheme EccSettingsDialog::uiToScheme() const
{
    EccScheme s;

    s.algo = static_cast<EccAlgorithm>(m_algo->currentData().toInt());
    s.sectorSize = m_sectorSize->currentData().toInt();
    s.oobSize = m_oobSize->value();
    s.m = m_m->value();
    s.t = m_t->value();
    s.primPoly = parsePoly(nullptr);
    s.eccBitOffset = m_eccBitOffset->value();
    s.coverSpare = m_coverSpare->isChecked();
    s.truncateTopBits = m_truncate->value();

    return s;
}

void EccSettingsDialog::refresh()
{
    const bool on = m_enable->isChecked();
    const EccScheme s = uiToScheme();
    const bool isBch = s.algo == ECC_ALGO_BCH;
    const bool isRs = s.algo == ECC_ALGO_RS;
    const bool fieldAlgo = isBch || isRs;

    /* Everything below the master switch follows it. */
    m_preset->setEnabled(on);
    m_presetDesc->setEnabled(false);
    m_advToggle->setEnabled(on);
    m_advanced->setEnabled(on);
    m_readReport->setEnabled(on);
    m_readCorrect->setEnabled(on);
    m_stopOnUncorrectable->setEnabled(on && m_readCorrect->isEnabled());
    m_generateOnWrite->setEnabled(on);

    /* Field parameters only mean something for BCH and Reed-Solomon. */
    QFormLayout *form = qobject_cast<QFormLayout *>(m_advanced->layout());
    if (form)
    {
        struct { QWidget *w; bool vis; } rows[] =
        {
            { m_rowM, fieldAlgo },
            { m_rowT, s.algo != ECC_ALGO_NONE && s.algo != ECC_ALGO_HAMMING },
            { m_rowPoly, fieldAlgo },
            { m_polyNote, fieldAlgo },
        };

        for (int i = 0; i < 4; i++)
        {
            QWidget *lbl = form->labelForField(rows[i].w);
            rows[i].w->setVisible(rows[i].vis);
            if (lbl)
                lbl->setVisible(rows[i].vis);
        }
    }

    const int presetIdx = m_preset->currentIndex();
    m_presetDesc->setText(presetIdx < EccScheme::presetCount() ?
        QString::fromUtf8(EccScheme::presetDescription(presetIdx)) :
        tr("Hand configured layout. Check it against the datasheet of the "
           "controller that wrote the device."));

    /* Live feedback on the polynomial, which is the field most likely to be
     * typed in by hand and the easiest to get wrong.
     */
    if (fieldAlgo)
    {
        bool parsed = true;
        const uint32_t entered = parsePoly(&parsed);
        const uint32_t eff = entered ? entered :
            GaloisField::defaultPrimPoly(s.m);
        GaloisField gf(s.m, entered);

        if (!parsed)
        {
            m_polyNote->setText(tr("Not a hexadecimal number."));
        }
        else if (!eff)
        {
            m_polyNote->setText(tr("No conventional polynomial for m = %1.")
                .arg(s.m));
        }
        else if (gf.isValid())
        {
            m_polyNote->setText(entered ?
                tr("0x%1 is primitive over GF(2^%2).")
                    .arg(eff, 0, 16).arg(s.m) :
                tr("Using the conventional 0x%1 for m = %2.")
                    .arg(eff, 0, 16).arg(s.m));
        }
        else
        {
            m_polyNote->setText(tr("0x%1 is not a primitive polynomial of "
                "degree %2.").arg(eff, 0, 16).arg(s.m));
        }
    }

    std::string why;
    bool valid;

    if (!on || s.algo == ECC_ALGO_NONE)
    {
        valid = true;
        why.clear();
    }
    else if (m_pageSize > 0)
    {
        valid = s.validateForPage(m_pageSize, m_spareSize, m_bbMarkOffset,
            why);
    }
    else
    {
        valid = s.validate(why);
    }

    const int bits = s.eccBits();
    const int sectors = m_pageSize > 0 ? s.sectorsPerPage(m_pageSize) : 0;

    if (!on || s.algo == ECC_ALGO_NONE)
    {
        m_summary->setText(tr("Pages are transferred unchanged."));
        m_strength->clear();
    }
    else
    {
        QString sum = tr("%1 parity bits (%2 bytes) per %3 byte sector")
            .arg(bits).arg(s.eccBytes()).arg(s.sectorSize);

        if (sectors > 0)
        {
            sum += tr(" · %1 sectors per page · %2 spare bytes used of %3")
                .arg(sectors).arg(sectors * s.oobSize).arg(m_spareSize);
        }

        m_summary->setText(sum);
        m_strength->setText(QString::fromStdString(
            EccEngine::strengthText(s)));
    }

    m_map->setScheme(s, valid);

    if (!on)
    {
        m_status->setText(QString());
    }
    else if (!m_spareIncluded)
    {
        /* Worth saying loudly: everything below can be configured correctly
         * and still do nothing, because the parity bytes never reach the host.
         */
        m_status->setStyleSheet("color: #C0392B;");
        m_status->setText(tr("\xE2\x9C\x95 \"Include spare area\" is off in "
            "Programmer settings, so parity never reaches the host and none of "
            "this takes effect. Turn it on first."));
    }
    else if (valid)
    {
        m_status->setStyleSheet("color: #2E7D32;");
        m_status->setText(m_pageSize > 0 ?
            tr("\xE2\x9C\x93 Valid for the selected chip.") :
            tr("\xE2\x9C\x93 Valid. Select a chip to check it fits the page."));
    }
    else
    {
        m_status->setStyleSheet("color: #C0392B;");
        m_status->setText(QString("\xE2\x9C\x95 ") +
            QString::fromStdString(why));
    }

    QPushButton *okBtn = m_buttons->button(QDialogButtonBox::Ok);
    if (okBtn)
        okBtn->setEnabled(valid);
}

/* ------------------------------------------------------------ accessors -- */

void EccSettingsDialog::setEccEnabled(bool enabled)
{
    m_enable->setChecked(enabled);
    refresh();
}

bool EccSettingsDialog::isEccEnabled() const
{
    return m_enable->isChecked();
}

void EccSettingsDialog::setScheme(const EccScheme &scheme)
{
    schemeToUi(scheme);
    refresh();
}

/* Always the scheme as configured, whether or not ECC is switched on, so the
 * caller can persist a layout the user spent time on and switch it back on
 * later without retyping it. Consult isEccEnabled() for whether to apply it.
 */
EccScheme EccSettingsDialog::scheme() const
{
    return uiToScheme();
}

void EccSettingsDialog::setVerifyOnRead(bool verify)
{
    if (!verify)
        m_readReport->setChecked(true);
    refresh();
}

bool EccSettingsDialog::isVerifyOnRead() const
{
    return m_enable->isChecked();
}

void EccSettingsDialog::setCorrectOnRead(bool correct)
{
    if (correct)
        m_readCorrect->setChecked(true);
    else
        m_readReport->setChecked(true);
    refresh();
}

bool EccSettingsDialog::isCorrectOnRead() const
{
    return m_readCorrect->isChecked();
}

void EccSettingsDialog::setGenerateOnWrite(bool generate)
{
    m_generateOnWrite->setChecked(generate);
}

bool EccSettingsDialog::isGenerateOnWrite() const
{
    return m_generateOnWrite->isChecked();
}

void EccSettingsDialog::setStopOnUncorrectable(bool stop)
{
    m_stopOnUncorrectable->setChecked(stop);
}

bool EccSettingsDialog::isStopOnUncorrectable() const
{
    return m_stopOnUncorrectable->isChecked();
}
