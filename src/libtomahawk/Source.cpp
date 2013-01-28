/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Source.h"

#include "Collection.h"
#include "SourceList.h"
#include "SourcePlaylistInterface.h"

#include "accounts/AccountManager.h"
#include "network/ControlConnection.h"
#include "database/DatabaseCommand_AddSource.h"
#include "database/DatabaseCommand_CollectionStats.h"
#include "database/DatabaseCommand_SourceOffline.h"
#include "database/DatabaseCommand_UpdateSearchIndex.h"
#include "database/Database.h"

#include <QCoreApplication>

#include "utils/TomahawkCache.h"
#include "database/DatabaseCommand_SocialAction.h"

#ifndef ENABLE_HEADLESS
    #include "utils/TomahawkUtilsGui.h"
#endif

#include "utils/Logger.h"
#include "sip/PeerInfo.h"

using namespace Tomahawk;


Source::Source( int id, const QString& nodeId )
    : QObject()
    , m_isLocal( false )
    , m_online( false )
    , m_nodeId( nodeId )
    , m_id( id )
    , m_updateIndexWhenSynced( false )
    , m_state( DBSyncConnection::UNKNOWN )
    , m_cc( 0 )
    , m_commandCount( 0 )
{
    m_scrubFriendlyName = qApp->arguments().contains( "--demo" );

    if ( id == 0 )
        m_isLocal = true;

    m_currentTrackTimer.setSingleShot( true );
    connect( &m_currentTrackTimer, SIGNAL( timeout() ), this, SLOT( trackTimerFired() ) );

    if ( m_isLocal )
    {
        connect( Accounts::AccountManager::instance(), SIGNAL( connected( Tomahawk::Accounts::Account* ) ), SLOT( setOnline() ) );
        connect( Accounts::AccountManager::instance(), SIGNAL( disconnected( Tomahawk::Accounts::Account* ) ), SLOT( setOffline() ) );
    }
}


Source::~Source()
{
    qDebug() << Q_FUNC_INFO << friendlyName();
}


void
Source::setControlConnection( ControlConnection* cc )
{
    m_cc = cc;
}

const QSet<peerinfo_ptr>
Source::peerInfos() const
{
    if ( controlConnection() )
    {
        return controlConnection()->peerInfos();
    }
    else if ( isLocal() )
    {
        return PeerInfo::getAllSelf().toSet();

    }
    return QSet< Tomahawk::peerinfo_ptr >();
}


collection_ptr
Source::dbCollection() const
{
    if ( m_collections.length() )
    {
        foreach ( const collection_ptr& collection, m_collections )
            if ( collection->type() == "databasecollection" )
                return collection; // We assume only one is a db collection. Now get off my lawn.
    }

    collection_ptr tmp;
    return tmp;
}


void
Source::setStats( const QVariantMap& m )
{
    m_stats = m;
    emit stats( m_stats );
    emit stateChanged();
}


QString
Source::friendlyName() const
{
    if ( m_friendlyname.isEmpty() )
        return m_nodeId;

    //TODO: this is a terrible assumption, help me clean this up, mighty muesli!
    if ( m_friendlyname.contains( "@conference." ) )
        return QString( m_friendlyname ).remove( 0, m_friendlyname.lastIndexOf( "/" ) + 1 ).append( " via MUC" );

    if ( m_friendlyname.contains( "/" ) )
        return m_friendlyname.left( m_friendlyname.indexOf( "/" ) );

    return m_friendlyname;
}


#ifndef ENABLE_HEADLESS
QPixmap
Source::avatar( TomahawkUtils::ImageMode style, const QSize& size )
{
//     tLog() << "****************************************************************************************";
//     tLog() << peerInfos().count() << "PEERS FOR " << friendlyName();
    QPixmap result;
    foreach( const peerinfo_ptr& peerInfo, peerInfos() )
    {
//         peerInfoDebug(peerInfo) << !peerInfo->avatar().isNull();
        if( !peerInfo.isNull() && !peerInfo->avatar( style, size ).isNull() )
        {
            result =  peerInfo->avatar( style, size );
            break;
        }
    }
//        tLog() << "****************************************************************************************";
    return result;
}
#endif


void
Source::setFriendlyName( const QString& fname )
{
    if ( fname.isEmpty() )
        return;

    m_friendlyname = fname;
    if ( m_scrubFriendlyName )
    {
        if ( m_friendlyname.indexOf( "@" ) > 0 )
            m_friendlyname = m_friendlyname.split( "@" ).first();
    }
}


void
Source::addCollection( const collection_ptr& c )
{
    Q_ASSERT( m_collections.length() == 0 ); // only 1 source supported atm
    m_collections.append( c );
    emit collectionAdded( c );
}


void
Source::removeCollection( const collection_ptr& c )
{
    Q_ASSERT( m_collections.length() == 1 && m_collections.first() == c ); // only 1 source supported atm
    m_collections.removeAll( c );
    emit collectionRemoved( c );
}


void
Source::setOffline()
{
    qDebug() << Q_FUNC_INFO << friendlyName();
    if ( !m_online )
        return;

    m_online = false;
    emit offline();

    if ( !isLocal() )
    {
        m_currentTrack.clear();
        emit stateChanged();

        m_cc = 0;
        DatabaseCommand_SourceOffline* cmd = new DatabaseCommand_SourceOffline( id() );
        Database::instance()->enqueue( QSharedPointer< DatabaseCommand >( cmd ) );
    }
}


void
Source::setOnline()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << friendlyName();
    if ( m_online )
        return;

    m_online = true;
    emit online();

    if ( !isLocal() )
    {
        // ensure username is in the database
        DatabaseCommand_addSource* cmd = new DatabaseCommand_addSource( m_nodeId, friendlyName() );
        connect( cmd, SIGNAL( done( unsigned int, QString ) ),
                        SLOT( dbLoaded( unsigned int, const QString& ) ) );
        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );
    }
}


void
Source::dbLoaded( unsigned int id, const QString& fname )
{
    m_id = id;
    setFriendlyName( fname );

    emit syncedWithDatabase();
}


void
Source::scanningProgress( unsigned int files )
{
    if ( files )
        m_textStatus = tr( "Scanning (%L1 tracks)" ).arg( files );
    else
        m_textStatus = tr( "Scanning" );

    emit stateChanged();
}


void
Source::scanningFinished( bool updateGUI )
{
    m_textStatus = QString();

    if ( m_updateIndexWhenSynced )
    {
        m_updateIndexWhenSynced = false;
        updateTracks();
    }

    emit stateChanged();

    if ( updateGUI )
        emit synced();
}


void
Source::onStateChanged( DBSyncConnection::State newstate, DBSyncConnection::State oldstate, const QString& info )
{
    Q_UNUSED( oldstate );

    QString msg;
    switch( newstate )
    {
        case DBSyncConnection::CHECKING:
        {
            msg = tr( "Checking" );
            break;
        }
        case DBSyncConnection::FETCHING:
        {
            msg = tr( "Syncing" );
            break;
        }
        case DBSyncConnection::PARSING:
        {
            msg = tr( "Importing" );
            break;
        }
        case DBSyncConnection::SCANNING:
        {
            msg = tr( "Scanning (%L1 tracks)" ).arg( info );
            break;
        }
        case DBSyncConnection::SYNCED:
        {
            msg = QString();
            break;
        }

        default:
            msg = QString();
    }

    m_state = newstate;
    m_textStatus = msg;
    emit stateChanged();
}


unsigned int
Source::trackCount() const
{
    return m_stats.value( "numfiles" ).toUInt();
}


Tomahawk::playlistinterface_ptr
Source::playlistInterface()
{
    if ( m_playlistInterface.isNull() )
    {
        Tomahawk::source_ptr source = SourceList::instance()->get( id() );
        m_playlistInterface = Tomahawk::playlistinterface_ptr( new Tomahawk::SourcePlaylistInterface( source.data() ) );
    }

    return m_playlistInterface;
}


void
Source::onPlaybackStarted( const Tomahawk::query_ptr& query, unsigned int duration )
{
    tLog( LOGVERBOSE ) << Q_FUNC_INFO << query->toString();

    m_currentTrack = query;
    m_currentTrackTimer.start( duration * 1000 + 900000 ); // duration comes in seconds

    if ( m_playlistInterface.isNull() )
        playlistInterface();

    emit playbackStarted( query );
    emit stateChanged();
}


void
Source::onPlaybackFinished( const Tomahawk::query_ptr& query )
{
    tDebug() << Q_FUNC_INFO << query->toString();
    emit playbackFinished( query );

    m_currentTrack.clear();
    emit stateChanged();
}


void
Source::trackTimerFired()
{
    m_currentTrack.clear();
    emit stateChanged();
}


QString
Source::lastCmdGuid() const
{
    QMutexLocker lock( &m_cmdMutex );
    return m_lastCmdGuid;
}


void
Source::addCommand( const QSharedPointer<DatabaseCommand>& command )
{
    QMutexLocker lock( &m_cmdMutex );

    m_cmds << command;
    if ( !command->singletonCmd() )
        m_lastCmdGuid = command->guid();

    m_commandCount = m_cmds.count();
}


void
Source::executeCommands()
{
    if ( QThread::currentThread() != thread() )
    {
        QMetaObject::invokeMethod( this, "executeCommands", Qt::QueuedConnection );
        return;
    }

    bool commandsAvail = false;
    {
        QMutexLocker lock( &m_cmdMutex );
        commandsAvail = !m_cmds.isEmpty();
    }

    if ( commandsAvail )
    {
        QMutexLocker lock( &m_cmdMutex );
        QList< QSharedPointer<DatabaseCommand> > cmdGroup;
        QSharedPointer<DatabaseCommand> cmd = m_cmds.takeFirst();
        while ( cmd->groupable() )
        {
            cmdGroup << cmd;
            if ( !m_cmds.isEmpty() && m_cmds.first()->groupable() && m_cmds.first()->commandname() == cmd->commandname() )
                cmd = m_cmds.takeFirst();
            else
                break;
        }

        // return here when the last command finished
        connect( cmd.data(), SIGNAL( finished() ), SLOT( executeCommands() ) );

        if ( cmdGroup.count() )
        {
            Database::instance()->enqueue( cmdGroup );
        }
        else
        {
            Database::instance()->enqueue( cmd );
        }

        int percentage = ( float( m_commandCount - m_cmds.count() ) / (float)m_commandCount ) * 100.0;
        m_textStatus = tr( "Saving (%1%)" ).arg( percentage );
        emit stateChanged();
    }
    else
    {
        if ( m_updateIndexWhenSynced )
        {
            m_updateIndexWhenSynced = false;
            updateTracks();
        }

        m_textStatus = QString();
        m_state = DBSyncConnection::SYNCED;

        emit commandsFinished();
        emit stateChanged();
        emit synced();
    }
}


void
Source::reportSocialAttributesChanged( DatabaseCommand_SocialAction* action )
{
    Q_ASSERT( action );

    emit socialAttributesChanged( action->action() );

    if ( action->action() == "latchOn" )
    {
        const source_ptr to = SourceList::instance()->get( action->comment() );
        if ( !to.isNull() )
            emit latchedOn( to );
    }
    else if ( action->action() == "latchOff" )
    {
        const source_ptr from = SourceList::instance()->get( action->comment() );
        if ( !from.isNull() )
            emit latchedOff( from );
    }
}


void
Source::updateTracks()
{
    {
        DatabaseCommand* cmd = new DatabaseCommand_UpdateSearchIndex();
        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
    }

    {
        // Re-calculate local db stats
        DatabaseCommand_CollectionStats* cmd = new DatabaseCommand_CollectionStats( SourceList::instance()->get( id() ) );
        connect( cmd, SIGNAL( done( QVariantMap ) ), SLOT( setStats( QVariantMap ) ), Qt::QueuedConnection );
        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
    }
}


void
Source::updateIndexWhenSynced()
{
    m_updateIndexWhenSynced = true;
}


QString
Source::textStatus() const
{
    if ( !m_textStatus.isEmpty() )
        return m_textStatus;

    if ( !currentTrack().isNull() )
    {
        return currentTrack()->artist() + " - " + currentTrack()->track();
    }

    // do not use isOnline() here - it will always return true for the local source
    if ( m_online )
    {
        return tr( "Online" );
    }
    else
    {
        return tr( "Offline" );
    }
}
