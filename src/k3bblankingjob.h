/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef K3B_BLANKING_JOB_H
#define K3B_BLANKING_JOB_H

#include "k3bjob.h"

class KProcess;
class QString;
class K3bCdDevice::CdDevice;


class K3bBlankingJob : public K3bJob
{
  Q_OBJECT

 public:
  K3bBlankingJob( QObject* parent = 0 );
  ~K3bBlankingJob();

  enum blank_mode { Fast, Complete, Track, Unclose, Session };

  bool active() const;

 public slots:
  void start();
  void cancel();
  void setForce( bool f ) { m_force = f; }
  void setDevice( K3bDevice* d );
  void setSpeed( int s ) { m_speed = s; }
  void setMode( int m ) { m_mode = m; }

 private slots:
  void slotParseCdrecordOutput( KProcess*, char*, int );
  void slotCdrecordFinished();
  void slotStartErasing();

 private:
  KProcess* m_process;
  bool m_force;
  K3bDevice* m_device;
  int m_speed;
  int m_mode;
};

#endif
