/***************************************************************************
                          k3bvcdtrack.h  -  description
                             -------------------
    begin                : Mon Nov 4 2002
    copyright            : (C) 2002 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3BVCDTRACK_H
#define K3BVCDTRACK_H

#include <qstring.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <qptrlist.h>

class K3bVcdTrack
{
 public:
  K3bVcdTrack( QList<K3bVcdTrack>* parent, const QString& filename );
  ~K3bVcdTrack();

  QString fileName() const { return QFileInfo(m_file).fileName(); }
  QString absPath() const { return QFileInfo(m_file).absFilePath(); }
  unsigned long size() const;
  int index() const;
  // video
  int mpegType() const {return m_mpegtype;}
  int mpegVideoVersion() const {return m_mpegvideoversion;}
  int mpegMuxRate() const {return m_mpegmuxrate;}
  int MpegFormat() const {return m_mpegformat;}
  QString mpegVersion() const {return m_mpegversion;}
  QString mpegDuration() const {return m_mpegduration;}
  QString mpegSize() const {return m_mpegsize;}
  QString mpegDisplaySize() const {return m_mpegdisplaysize;}
  QString mpegFps() const {return m_mpegfps;}
  QString mpegMbps() const {return m_mpegmbps;}
  int MpegAspectRatio() const {return m_mpegaspect_ratio;}
  bool MpegSExt() const {return m_mpegprogressive;}
  bool MpegDExt() const {return m_mpegsext;}
  bool MpegProgressive() const {return m_mpegdext;}
  int MpegChromaFormat() const {return m_mpegchroma_format;}

  // audio
  bool hasAudio() const {return m_hasaudio;}
  int MpegAudioType() const {return m_mpegaudiotype;}
  int MpegAudioLayer() const {return m_mpegaudiolayer;}
  QString MpegAudioDuration() const {return m_mpegaudioduration;}
  QString MpegAudioKbps() const {return m_mpegaudiokbps;}
  QString MpegAudioHz() const {return m_mpegaudiohz;}
  QString MpegAudioFrame() const {return m_mpegaudioframe;}
  int MpegAudioMode() const {return m_mpegaudiomode;}
  int MpegAudioModeExt() const {return m_mpegaudiomodeext;}
  int MpegAudioEmphasis() const {return m_mpegaudioemphasis;}
  // video
  void setMpegType(const int&);
  void setMpegVideoVersion(const int&);
  void setMpegMuxRate(const int&);
  void setMpegFormat(const int&);
  void setMpegVersion(const QString&);
  void setMpegDuration(const QString&);
  void setMpegSize(const QString&);
  void setMpegDisplaySize(const QString&);
  void setMpegFps(const QString&);
  void setMpegMbps(const QString&);
  void setMpegAspectRatio(const int&);
  void setMpegProgressive(const bool&);
  void setMpegSExt(const bool&);
  void setMpegDExt(const bool&);
  void setMpegChromaFormat(const int&);
  // audio
  void setHasAudio(const bool&);
  void setMpegAudioType(const int&);
  void setMpegAudioLayer(const int&);
  void setMpegAudioDuration(const QString&);
  void setMpegAudioKbps(const QString&);
  void setMpegAudioHz(const QString&);
  void setMpegAudioFrame(const QString&);
  void setMpegAudioMode(const int&);
  void setMpegAudioModeExt(const int&);
  void setMpegAudioEmphasis(const int&);
  
 protected:
  QList<K3bVcdTrack>* m_parent;
  int m_filetype;
  QFile m_file;
  // video
  int m_mpegtype;
  int m_mpegvideoversion;
  int m_mpegmuxrate;
  int m_mpegformat;
  QString m_mpegversion;
  QString m_mpegduration;
  QString m_mpegsize;
  QString m_mpegdisplaysize;
  QString m_mpegfps;
  QString m_mpegmbps;
  int m_mpegaspect_ratio;
  bool m_mpegprogressive;
  bool m_mpegsext;
  bool m_mpegdext;
  int m_mpegchroma_format;
  // audio
  bool m_hasaudio;
  int m_mpegaudiotype;
  int m_mpegaudiolayer;
  QString m_mpegaudioduration;
  QString m_mpegaudiokbps;
  QString m_mpegaudiohz;
  QString m_mpegaudioframe;
  int m_mpegaudiomode;
  int m_mpegaudiomodeext;
  int m_mpegaudioemphasis;
};


#endif
