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

#include <config.h>


#include "k3bcdrecordwriter.h"

#include <k3bcore.h>
#include <k3bexternalbinmanager.h>
#include <k3bprocess.h>
#include <device/k3bdevice.h>
#include <device/k3bdevicemanager.h>
#include <device/k3bdevicehandler.h>
#include <k3bglobals.h>
#include <k3bthroughputestimator.h>

#include <qstring.h>
#include <qstringlist.h>
#include <qurl.h>
#include <qvaluelist.h>
#include <qregexp.h>

#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kmessagebox.h>

#include <errno.h>
#include <string.h>



class K3bCdrecordWriter::Private
{
public:
  Private() 
    : running(false){
  }

  K3bThroughputEstimator* speedEst;
  bool running;
  bool usingBurnfree;
  int usedSpeed;
};


K3bCdrecordWriter::K3bCdrecordWriter( K3bDevice* dev, QObject* parent, const char* name )
  : K3bAbstractWriter( dev, parent, name ),
    m_stdin(false),
    m_clone(false),
    m_forceNoEject(false)
{
  d = new Private();
  d->speedEst = new K3bThroughputEstimator( this );
  connect( d->speedEst, SIGNAL(throughput(int)),
	   this, SLOT(slotThroughput(int)) );

  m_process = 0;
  m_writingMode = K3b::TAO;
}


K3bCdrecordWriter::~K3bCdrecordWriter()
{
  delete d;
  delete m_process;
}


bool K3bCdrecordWriter::active() const
{
  return d->running;
}


int K3bCdrecordWriter::fd() const
{
  if( m_process )
    return m_process->stdinFd();
  else
    return -1;
}


void K3bCdrecordWriter::setDao( bool b )
{
  m_writingMode = ( b ? K3b::DAO : K3b::TAO );
}

void K3bCdrecordWriter::setCueFile( const QString& s)
{
  m_cue = true;
  m_cueFile = s;
}
  
void K3bCdrecordWriter::setClone( bool b )
{
  m_clone = b;
}


void K3bCdrecordWriter::setWritingMode( int mode )
{
  if( mode == K3b::DAO ||
      mode == K3b::TAO ||
      mode == K3b::RAW )
    m_writingMode = mode;
  else 
    kdError() << "(K3bCdrecordWriter) wrong writing mode: " << mode << endl;
}


void K3bCdrecordWriter::prepareProcess()
{
  if( m_process ) delete m_process;  // kdelibs want this!
  m_process = new K3bProcess();
  m_process->setRunPrivileged(true);
  m_process->setSplitStdout(true);
  m_process->setSuppressEmptyLines(true);
  connect( m_process, SIGNAL(stdoutLine(const QString&)), this, SLOT(slotStdLine(const QString&)) );
  connect( m_process, SIGNAL(stderrLine(const QString&)), this, SLOT(slotStdLine(const QString&)) );
  connect( m_process, SIGNAL(processExited(KProcess*)), this, SLOT(slotProcessExited(KProcess*)) );
  connect( m_process, SIGNAL(wroteStdin(KProcess*)), this, SIGNAL(dataWritten()) );

  m_cdrecordBinObject = k3bcore->externalBinManager()->binObject("cdrecord");

  if( !m_cdrecordBinObject )
    return;

  k3bcore->config()->setGroup("General Options");

  *m_process << m_cdrecordBinObject->path;

  // display progress
  *m_process << "-v";
    
  if( m_cdrecordBinObject->hasFeature( "gracetime") )
    *m_process << "gracetime=2";  // 2 is the lowest allowed value (Joerg, why do you do this to us?)
    
  // Again we assume the device to be set!
  *m_process << QString("dev=%1").arg(K3bCdDevice::externalBinDeviceParameter(burnDevice(), m_cdrecordBinObject));

  d->usedSpeed = burnSpeed();
  if( d->usedSpeed == 0 ) {
    // try to determine the writeSpeed
    // if it fails determineMaximalWriteSpeed() will return 0 and
    // the choice is left to cdrecord
    d->usedSpeed = burnDevice()->determineMaximalWriteSpeed()/175;
  }
  if( d->usedSpeed != 0 )
    *m_process << QString("speed=%1").arg(d->usedSpeed);
    
  if( m_writingMode == K3b::DAO ) {
    if( burnDevice()->dao() )
      *m_process << "-dao";
    else
      emit infoMessage( i18n("Writer does not support disk at once (DAO) recording"), WARNING );
  }
  else if( m_writingMode == K3b::RAW ) {
      *m_process << "-raw";
  }
    
  if( simulate() )
    *m_process << "-dummy";
    
  d->usingBurnfree = false;
  if( burnproof() ) {
    if( burnDevice()->burnproof() ) {

      d->usingBurnfree = true;

      // with cdrecord 1.11a02 burnproof was renamed to burnfree
      if( m_cdrecordBinObject->version < K3bVersion( "1.11a02" ) )
	*m_process << "driveropts=burnproof";
      else
	*m_process << "driveropts=burnfree";
    }
    else
      emit infoMessage( i18n("Writer does not support buffer underrun free recording (BURNPROOF)"), WARNING );
  }
  
  if ( m_cue && !m_cueFile.isEmpty() ) {
      m_process->setWorkingDirectory(QUrl(m_cueFile).dirPath());
    *m_process << QString("cuefile=%1").arg( m_cueFile );
  }
  
  if( m_clone )
    *m_process << "-clone";
  
  if( !k3bcore->config()->readBoolEntry( "No cd eject", false ) &&
      !m_forceNoEject )
    *m_process << "-eject";

  bool manualBufferSize = k3bcore->config()->readBoolEntry( "Manual buffer size", false );
  if( manualBufferSize ) {
    *m_process << QString("fs=%1m").arg( k3bcore->config()->readNumEntry( "Cdrecord buffer", 4 ) );
  }
    
  bool overburn = k3bcore->config()->readBoolEntry( "Allow overburning", false );
  if( overburn )
    if( m_cdrecordBinObject->hasFeature("overburn") )
      *m_process << "-overburn";
    else
      emit infoMessage( i18n("Cdrecord %1 does not support overburning!").arg(m_cdrecordBinObject->version), WARNING );
    
  // additional user parameters from config
  const QStringList& params = m_cdrecordBinObject->userParameters();
  for( QStringList::const_iterator it = params.begin(); it != params.end(); ++it )
    *m_process << *it;

  // add the user parameters
  for( QStringList::const_iterator it = m_arguments.begin(); it != m_arguments.end(); ++it )
    *m_process << *it;
}


K3bCdrecordWriter* K3bCdrecordWriter::addArgument( const QString& arg )
{
  m_arguments.append( arg );
  return this;
}


void K3bCdrecordWriter::clearArguments()
{
  m_arguments.clear();
}


void K3bCdrecordWriter::start()
{
  emit started();

  d->running = true;
  d->speedEst->reset();

  prepareProcess();

  if( !m_cdrecordBinObject ) {
    emit infoMessage( i18n("Could not find %1 executable.").arg("cdrecord"), ERROR );
    emit finished(false);
    d->running = false;
    return;
  }

  if( !m_cdrecordBinObject->copyright.isEmpty() )
    emit infoMessage( i18n("Using %1 %2 - Copyright (C) %3").arg(m_cdrecordBinObject->name()).arg(m_cdrecordBinObject->version).arg(m_cdrecordBinObject->copyright), INFO );


  kdDebug() << "***** " << m_cdrecordBinObject->name() << " parameters:\n";
  const QValueList<QCString>& args = m_process->args();
  QString s;
  for( QValueList<QCString>::const_iterator it = args.begin(); it != args.end(); ++it ) {
    s += *it + " ";
  }
  kdDebug() << s << flush << endl;
  emit debuggingOutput( m_cdrecordBinObject->name() + " comand:", s);

  m_currentTrack = 0;
  m_cdrecordError = UNKNOWN;
  m_totalTracksParsed = false;
  m_alreadyWritten = 0;
  m_trackSizes.clear();
  m_totalSize = 0;

  emit newSubTask( i18n("Preparing write process...") );

  if( !m_process->start( KProcess::NotifyOnExit, KProcess::All ) ) {
    // something went wrong when starting the program
    // it "should" be the executable
    kdDebug() << "(K3bCdrecordWriter) could not start " << m_cdrecordBinObject->name() << endl;
    emit infoMessage( i18n("Could not start %1.").arg(m_cdrecordBinObject->name()), K3bJob::ERROR );
    d->running = false;
    emit finished(false);
  }
  else {
    if( simulate() ) {
      emit newTask( i18n("Simulating") );
      if( m_writingMode == K3b::DAO )
	emit infoMessage( i18n("Starting dao simulation at %1x speed...").arg(d->usedSpeed), 
			  K3bJob::INFO );
      else if( m_writingMode == K3b::RAW )
	emit infoMessage( i18n("Starting raw simulation at %1x speed...").arg(d->usedSpeed), 
			  K3bJob::INFO );
      else
	emit infoMessage( i18n("Starting tao simulation at %1x speed...").arg(d->usedSpeed), 
			  K3bJob::INFO );
    }
    else {
      emit newTask( i18n("Writing") );

      if( m_writingMode == K3b::DAO )
	emit infoMessage( i18n("Starting dao writing at %1x speed...").arg(d->usedSpeed), K3bJob::INFO );
      else if( m_writingMode == K3b::RAW )
	emit infoMessage( i18n("Starting raw writing at %1x speed...").arg(d->usedSpeed), K3bJob::INFO );
      else
	emit infoMessage( i18n("Starting tao writing at %1x speed...").arg(d->usedSpeed), K3bJob::INFO );
    }
  }
}


void K3bCdrecordWriter::cancel()
{
  if( active() ) {
    if( m_process ) {
      if( m_process->isRunning() ) {
	m_process->disconnect();
	m_process->kill();
	
	// this will unblock and eject the drive and emit the finished/canceled signals
	K3bAbstractWriter::cancel();
      }
    }
  }
}


bool K3bCdrecordWriter::write( const char* data, int len )
{
  if( m_process )
    return m_process->writeStdin( data, len );
  else
    return -1;
}


void K3bCdrecordWriter::slotStdLine( const QString& line )
{
  emit debuggingOutput( m_cdrecordBinObject->name(), line );
  
  if( line.startsWith( "Track " ) )
    {
      if( !m_totalTracksParsed ) {
	// this is not the progress display but the list of tracks that will get written
	// we always extract the tracknumber to get the highest at last
	bool ok;
	int tt = line.mid( 6, 2 ).toInt(&ok);
	if( ok ) {
	  m_totalTracks = tt;

	  int sizeStart = line.find( QRegExp("\\d"), 10 );
	  int sizeEnd = line.find( "MB", sizeStart );
	  int ts = line.mid( sizeStart, sizeEnd-sizeStart ).toInt(&ok);
	  
	  if( ok ) {
	    m_trackSizes.append(ts);
	    m_totalSize += ts;
	  }
	  else
	    kdDebug() << "(K3bCdrecordWriter) track number parse error: " 
		      << line.mid( sizeStart, sizeEnd-sizeStart ) << endl;
	}
	else
	  kdDebug() << "(K3bCdrecordWriter) track number parse error: " 
		    << line.mid( 6, 2 ) << endl;
      }

      else if( line.contains( "fifo", false ) > 0 )
	{
	  // parse progress
	  int num, made, size, fifo;
	  bool ok;

	  // --- parse number of track ---------------------------------------
	  // ----------------------------------------------------------------------
	  int pos1 = 5;
	  int pos2 = line.find(':');
	  if( pos1 == -1 ) {
	    kdDebug() << "parsing did not work" << endl;
	    return;
	  }
	  // now pos2 to the first colon :-)
	  num = line.mid(pos1,pos2-pos1).toInt(&ok);
	  if(!ok)
	    kdDebug() << "parsing did not work" << endl;

	  // --- parse already written Megs -----------------------------------
	  // ----------------------------------------------------------------------
	  pos1 = line.find(':');
	  if( pos1 == -1 ) {
	    kdDebug() << "parsing did not work" << endl;
	    return;
	  }
	  pos2 = line.find("of");
	  if( pos2 == -1 ) {
	    kdDebug() << "parsing did not work" << endl;
	    return;
	  }
	  // now pos1 point to the colon and pos2 to the 'o' of 'of' :-)
	  pos1++;
	  made = line.mid(pos1,pos2-pos1).toInt(&ok);
	  if(!ok)
	    kdDebug() << "parsing did not work" << endl;

	  // --- parse total size of track ---------------------------------------
	  // ------------------------------------------------------------------------
	  pos1 = line.find("MB");
	  if( pos1 == -1 ) {
	    kdDebug() << "parsing did not work" << endl;
	    return;
	  }
	  // now pos1 point to the 'M' of 'MB' and pos2 to the 'o' of 'of' :-)
	  pos2 += 2;
	  size = line.mid(pos2,pos1-pos2).toInt(&ok);
	  if(!ok)
	    kdDebug() << "parsing did not work" << endl;

	  // --- parse status of fifo --------------------------------------------
	  // ------------------------------------------------------------------------
	  pos1 = line.find("fifo");
	  if( pos1 == -1 ) {
	    kdDebug() << "parsing did not work" << endl;
	    return;
	  }
	  pos2 = line.find('%');
	  if( pos2 == -1 ) {
	    kdDebug() << "parsing did not work" << endl;
	    return;
	  }
	  // now pos1 point to the 'f' of 'fifo' and pos2 to the %o'  :-)
	  pos1+=4;
	  fifo = line.mid(pos1,pos2-pos1).toInt(&ok);
	  if(!ok)
	    kdDebug() << "parsing did not work" << endl;

	  // -------------------------------------------------------------------
	  // -------- parsing finished --------------------------------------


	  emit buffer( fifo );
	  m_lastFifoValue = fifo;

	  //
	  // cdrecord's output sucks a bit.
	  // we get track sizes that differ from the sizes in the progress
	  // info since these are dependant on the writing mode.
	  // so we just use the track sizes and do a bit of math...
	  //

	  if( (int)m_trackSizes.count() > m_currentTrack-1 &&
	      size > 0 ) {
	    double convV = (double)m_trackSizes[m_currentTrack-1]/(double)size;
	    made = (int)((double)made * convV);
	    size = m_trackSizes[m_currentTrack-1];
	  }
	  else {
	    kdError() << "(K3bCdrecordWriter) Did not parse all tracks sizes!" << endl;
	  }

	  if( size > 0 ) {
	    emit processedSubSize( made, size );
	    emit subPercent( 100*made/size );
	  }

	  if( m_totalSize > 0 ) {
	    emit processedSize( m_alreadyWritten+made, m_totalSize );
	    emit percent( 100*(m_alreadyWritten+made)/m_totalSize );
	  }

	  d->speedEst->dataWritten( (m_alreadyWritten+made)*1024 );
	}
    }
  else if( line.contains( "at speed" ) ) {
    // parse the speed and inform the user if cdrdao switched it down
    int pos = line.find( "at speed" );
    int po2 = line.find( QRegExp("\\D"), pos + 9 );
    int speed = line.mid( pos+9, po2-pos-9 ).toInt();
    if( speed < d->usedSpeed ) {
      emit infoMessage( i18n("Medium or burner do not support writing at %1x speed").arg(d->usedSpeed), K3bJob::WARNING );
      emit infoMessage( i18n("Switching down burn speed to %1x").arg(speed), K3bJob::WARNING );
    }
  }
  else if( line.startsWith( "Starting new" ) ) {
    m_totalTracksParsed = true;
    if( m_currentTrack > 0 ) {// nothing has been written at the start of track 1
      if( (int)m_trackSizes.count() > m_currentTrack-1 )
	m_alreadyWritten += m_trackSizes[m_currentTrack-1];
      else
	kdError() << "(K3bCdrecordWriter) Did not parse all tracks sizes!" << endl;
    }
    else
      emit infoMessage( i18n("Starting writing"), INFO );

    m_currentTrack++;
    kdDebug() << "(K3bCdrecordWriter) writing track " << m_currentTrack << " of " << m_totalTracks << " tracks." << endl;
    emit nextTrack( m_currentTrack, m_totalTracks );
  }
  else if( line.startsWith( "Fixating" ) ) {
    emit newSubTask( i18n("Fixating") );
  }
  else if( line.contains("seconds.") ) {
    int pos2 = line.find("seconds.") - 2;
    int pos1 = line.findRev( QRegExp("\\D"), pos2 ) + 1;
    int secs = line.mid(pos1, pos2-pos1+1).toInt();
    if( secs > 0 )
      emit infoMessage( i18n("Starting in 1 second", 
			     "Starting in %n seconds", 
			     secs), K3bJob::INFO );
  }
  else if( line.startsWith( "Writing lead-in" ) ) {
    m_totalTracksParsed = true;
    emit newSubTask( i18n("Writing Leadin") );
  }
  else if( line.startsWith( "Writing Leadout") ) {
    emit newSubTask( i18n("Writing Leadout") );
  }
  else if( line.startsWith( "Writing pregap" ) ) {
    emit newSubTask( i18n("Writing pregap") );
  }
  else if( line.startsWith( "Performing OPC" ) ) {
    emit infoMessage( i18n("Performing Optimum Power Calibration"), K3bJob::INFO );
  }
  else if( line.startsWith( "Sending" ) ) {
    emit infoMessage( i18n("Sending CUE sheet"), K3bJob::INFO );
  }
  else if( line.contains( "Turning BURN-Proof" ) ) {
    emit infoMessage( i18n("Enabled BURN-Proof"), K3bJob::INFO );
  }
  else if( line.contains( "Drive needs to reload the media" ) ) {
    emit infoMessage( i18n("Reloading of media required"), K3bJob::INFO );
  }
  else if( line.contains( "The current problem looks like a buffer underrun" ) ) {
    m_cdrecordError = BUFFER_UNDERRUN;
  }
  else if( line.contains( "Drive does not support SAO" ) ) {
    emit infoMessage( i18n("DAO (Disk At Once) recording not supported with this writer"), K3bJob::ERROR );
    emit infoMessage( i18n("Please choose TAO (Track At Once) and try again"), K3bJob::ERROR );
  }
  else if( line.contains("Data may not fit") ) {
    bool overburn = k3bcore->config()->readBoolEntry( "Allow overburning", false );
    if( overburn && m_cdrecordBinObject->hasFeature("overburn") )
      emit infoMessage( i18n("Trying to write more than the official disk capacity"), K3bJob::WARNING );
    m_cdrecordError = OVERSIZE;
  }
  else if( line.contains("Bad Option") ) {
    m_cdrecordError = BAD_OPTION;
    // parse option
    int pos = line.find( "Bad Option" ) + 13;
    int len = line.length() - pos - 1;
    emit infoMessage( i18n("No valid %1 option: %2").arg(m_cdrecordBinObject->name()).arg(line.mid(pos, len)), 
		      ERROR );
  }
  else if( line.contains("shmget failed") ) {
    m_cdrecordError = SHMGET_FAILED;
  }
  else if( line.contains("OPC failed") ) {
    m_cdrecordError = OPC_FAILED;
  }
  else if( line.contains("Cannot set speed/dummy") ) {
    m_cdrecordError = CANNOT_SET_SPEED;
  }
  else if( line.contains("Cannot open new session") ) {
    m_cdrecordError = CANNOT_OPEN_NEW_SESSION;
  }
  else if( line.contains("Cannot send CUE sheet") ) {
    m_cdrecordError = CANNOT_SEND_CUE_SHEET;
  }
  else if( line.contains("Input/output error.") ) {
    emit infoMessage( i18n("Input/output error. Not necessarily serious."), WARNING );
  }
  else if( line.contains( "Permission denied. Cannot open" ) ) {
    m_cdrecordError = PERMISSION_DENIED;
  }
  else if( line.contains( "Trying to use ultra high speed medium on improper writer" ) ) {
    m_cdrecordError = HIGH_SPEED_MEDIUM;
  }
  else if( line.startsWith( "Re-load disk and hit" ) ) {
    // this happens on some notebooks where cdrecord is not able to close the
    // tray itself, so we need to ask the user to do so
    KMessageBox::information( 0, i18n("Please reload the medium and press 'ok'"),
			      i18n("Unable to close the tray") );

    // now send a <CR> to cdrecord
    // hopefully this will do it since I have no possibility to test it!
    m_process->writeStdin( "\n", 1 );
  }
  else {
    // debugging
    kdDebug() << "(" << m_cdrecordBinObject->name() << ") " << line << endl;
  }
}


void K3bCdrecordWriter::slotProcessExited( KProcess* p )
{
  if( p->normalExit() ) {
    switch( p->exitStatus() ) {
    case 0:
      {
	if( simulate() )
	  emit infoMessage( i18n("Simulation successfully finished"), K3bJob::SUCCESS );
	else
	  emit infoMessage( i18n("Writing successfully finished"), K3bJob::SUCCESS );
	
	int s = d->speedEst->average();
	emit infoMessage( i18n("Average overall write speed: %1 KB/s (%2x)").arg(s).arg(KGlobal::locale()->formatNumber((double)s/150.0), 2), INFO );
	
	d->running = false;
	emit finished( true );
      }
      break;

    default:
      kdDebug() << "(K3bCdrecordWriter) error: " << p->exitStatus() << endl;

      if( m_cdrecordError == UNKNOWN && m_lastFifoValue <= 3 )
	m_cdrecordError = BUFFER_UNDERRUN;

      switch( m_cdrecordError ) {
      case OVERSIZE:
	if( k3bcore->config()->readBoolEntry( "Allow overburning", false ) &&
	    m_cdrecordBinObject->hasFeature("overburn") )
	  emit infoMessage( i18n("Data did not fit on disk."), ERROR );
	else
	  emit infoMessage( i18n("Data does not fit on disk."), ERROR );
	break;
      case BAD_OPTION:
	// error message has already been emited earlier since we needed the actual line
	break;
      case SHMGET_FAILED:
	emit infoMessage( i18n("%1 could not reserve shared memory segment of requested size.").arg(m_cdrecordBinObject->name()), ERROR );
	emit infoMessage( i18n("Probably you chose a too large buffer size."), ERROR );
	break;
      case OPC_FAILED:
	emit infoMessage( i18n("OPC failed. Probably the writer does not like the medium."), ERROR );
	break;
      case CANNOT_SET_SPEED:
	emit infoMessage( i18n("Unable to set write speed to %1.").arg(d->usedSpeed), ERROR );
	emit infoMessage( i18n("Probably this is lower than your writer's lowest writing speed."), ERROR );
	break;
      case CANNOT_SEND_CUE_SHEET:
	emit infoMessage( i18n("Unable to send CUE sheet."), ERROR );
	emit infoMessage( i18n("This may be caused by wrong settings."), ERROR );
	break;
      case CANNOT_OPEN_NEW_SESSION:
	emit infoMessage( i18n("Unable to open new session."), ERROR );
	emit infoMessage( i18n("Probably a problem with the medium."), ERROR );
	break;
      case PERMISSION_DENIED:
	emit infoMessage( i18n("%1 has no permission to open the device.").arg("Cdrecord"), ERROR );
	emit infoMessage( i18n("You may use K3bsetup2 to solve this problem."), ERROR );
	break;
      case BUFFER_UNDERRUN:
	  emit infoMessage( i18n("Probably a buffer underrun occurred."), ERROR );
	  if( !d->usingBurnfree && burnDevice()->burnproof() )
	    emit infoMessage( i18n("Please enable Burnfree or choose a lower burning speed."), ERROR );
	  else
	    emit infoMessage( i18n("Please choose a lower burning speed."), ERROR );
	break;
      case HIGH_SPEED_MEDIUM:
	emit infoMessage( i18n("Found a high speed medium not suitable for the writer being used."), ERROR );
	break;
      case UNKNOWN:
	emit infoMessage( i18n("%1 returned an unknown error (code %2).").arg(m_cdrecordBinObject->name()).arg(p->exitStatus()), 
			  K3bJob::ERROR );
	emit infoMessage( strerror(p->exitStatus()), K3bJob::ERROR );

	if( !m_cdrecordBinObject->hasFeature( "suidroot" ) ) {
	  emit infoMessage( i18n("Cdrecord is not being run with root privileges."), ERROR );
	  emit infoMessage( i18n("This influences the stability of the burning process."), ERROR );
#ifdef HAVE_K3BSETUP
	  emit infoMessage( i18n("Use K3bSetup to solve this problem."), ERROR );    
#endif
	}
	else {
	  emit infoMessage( i18n("Please send me an email with the last output."), K3bJob::ERROR );
	}
	break;
      }
      d->running = false;
      emit finished( false );
    }
  }
  else {
    emit infoMessage( i18n("%1 did not exit cleanly.").arg(m_cdrecordBinObject->name()), 
		      ERROR );
    d->running = false;
    emit finished( false );
  }
}


void K3bCdrecordWriter::slotThroughput( int t )
{
  emit writeSpeed( t, 150 );
}

#include "k3bcdrecordwriter.moc"
