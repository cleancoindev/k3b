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



#include "k3bglobals.h"

#include <kglobal.h>
#include <kstandarddirs.h>

#include <qdatastream.h>

#include <cmath>

struct Sample {
    unsigned char msbLeft;
    unsigned char lsbLeft;
    unsigned char msbRight;
    unsigned char lsbRight;

    short left (  ) const {
        return ( msbLeft << 8 ) | lsbLeft;
    } short right (  ) const {
        return ( msbRight << 8 ) | lsbRight;
    } void left ( short d ) {
        msbLeft = d >> 8;
        lsbLeft = d;
    } void right ( short d ) {
        msbRight = d >> 8;
        lsbRight = d;
    }
};

QString K3b::framesToString( int h, bool showFrames )
{
  int m = h / 4500;
  int s = (h % 4500) / 75;
  int f = h % 75;

  QString str;

  if( showFrames ) {
    // cdrdao needs the MSF format where 1 second has 75 frames!
    str.sprintf( "%.2i:%.2i:%.2i", m, s, f );
  }
  else
    str.sprintf( "%.2i:%.2i", m, s );

  return str;
}

QString K3b::sizeToTime(long size){
	int h = size / sizeof(Sample) / 588;
	return framesToString(h, false);
}


Q_INT16 K3b::swapByteOrder( Q_INT16 i )
{
  return ((i << 8) & 0xff00) | ((i >> 8 ) & 0xff);
}


Q_INT32 K3b::swapByteOrder( Q_INT32 i )
{
  return ((i << 24) & 0xff000000) | ((i << 8) & 0xff0000) | ((i >> 8) & 0xff00) | ((i >> 24) & 0xff );
}


int K3b::round( double d )
{
  return (int)( floor(d) + 0.5 <= d ? ceil(d) : floor(d) );
}


QString K3b::globalConfig()
{
  // this is a little not to hard hack to ensure that we get the "global" k3b appdir
  // k3b_cd_copy.png should always be in $KDEDIR/share/apps/k3b/pics/
  return KGlobal::dirs()->findResourceDir( "data", "k3b/pics/k3b_cd_copy.png" ) + "k3b/k3bsetup";
}
