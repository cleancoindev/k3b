/***************************************************************************
                          k3baudiojob.h  -  description
                             -------------------
    begin                : Fri May 4 2001
    copyright            : (C) 2001 by Sebastian Trueg
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

#ifndef K3BAUDIOJOB_H
#define K3BAUDIOJOB_H

#include "../k3bjob.h"

class K3bAudioDoc;
class K3bAudioTrack;
class QString;


#include <qlist.h>
#include <kprocess.h>
#include <kurl.h>


/**
  *@author Sebastian Trueg
  */

class K3bAudioJob : public K3bBurnJob  {

  Q_OBJECT

 public:
  K3bAudioJob( K3bAudioDoc* doc );
  ~K3bAudioJob();

  K3bDoc* doc() const;
  K3bDevice* writer() const;
	
 public slots:
  void start();
  void cancel();
	
 protected slots:
  void slotParseCdrecordOutput( KProcess*, char*, int );
  void slotParseCdrdaoOutput( KProcess*, char* output, int len );
  void slotCdrecordFinished();
  void slotCdrdaoFinished();
  void slotEmitProgress( int trackMade, int TrackSize );
  void slotModuleFinished( bool );
	
 private:
  class SAudioTrackInfo {
  public:
    SAudioTrackInfo( K3bAudioTrack* t, const KURL& url )
      :urlToDecodedWav( url )
      {
	track = t;
	decoded = false;
      }
    K3bAudioTrack* track;
    KURL urlToDecodedWav;
    bool decoded;
  };
  QList<SAudioTrackInfo> m_trackInfoList;
  SAudioTrackInfo* m_currentTrackInfo;

  void createTrackInfo();
  void clearBufferFiles();
  void decodeNextFile();
  void startWriting();
	
  KShellProcess m_process;
  K3bAudioDoc* m_doc;
  const K3bAudioTrack* m_currentProcessedTrack;
  bool firstTrack;
  QString m_tocFile;
  int m_iNumTracksAlreadyWritten;
  int m_iNumFilesToDecode;
  int m_iNumFilesAlreadyDecoded;
  int m_iTracksAlreadyWrittenSize;
  int m_iDocSize;

 signals:
  void writingLeadOut();
};

#endif
