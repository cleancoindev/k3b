/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *                    Klaus-Dieter Krannich <kd@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BCDCOPYDIALOG_H
#define K3BCDCOPYDIALOG_H


#include <k3binteractiondialog.h>

namespace K3bCdDevice {
  class CdDevice;
}

class K3bWriterSelectionWidget;
class K3bTempDirSelectionWidget;
class K3bDeviceComboBox;
class QCheckBox;
class QSpinBox;
class QComboBox;
class K3bWritingModeWidget;
class QButtonGroup;
class QRadioButton;


/**
  *@author Sebastian Trueg
  */
class K3bCdCopyDialog : public K3bInteractionDialog
{
  Q_OBJECT

 public: 
  K3bCdCopyDialog(QWidget *parent = 0, const char *name = 0, bool modal = true );
  ~K3bCdCopyDialog();

  K3bCdDevice::CdDevice* readingDevice() const;

 private slots:
  void slotStartClicked();

  void slotToggleAll();

  void slotLoadUserDefaults();
  void slotSaveUserDefaults();
  void slotLoadK3bDefaults();

 private:
  K3bWriterSelectionWidget* m_writerSelectionWidget;
  K3bTempDirSelectionWidget* m_tempDirSelectionWidget;
  QCheckBox* m_checkSimulate;
  QCheckBox* m_checkBurnfree;
  QCheckBox* m_checkOnTheFly;
  QCheckBox* m_checkDeleteImages;
  QCheckBox* m_checkOnlyCreateImage;
  QCheckBox* m_checkPrefereCdText;
  QCheckBox* m_checkIgnoreReadErrors;
  QCheckBox* m_checkNoCorrection;
  K3bDeviceComboBox* m_comboSourceDevice;
  QComboBox* m_comboParanoiaMode;
  QSpinBox* m_spinCopies;
  QSpinBox* m_spinRetries;
  K3bWritingModeWidget* m_writingModeWidget;
  QButtonGroup* m_groupCopyMode;
  QRadioButton* m_radioNormalCopy;
  QRadioButton* m_radioCloneCopy;
};

#endif
