/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
 *   Copyright 2011, Dominik Schmidt <dev@dominik-schmidt.de>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "SipInfo.h"

#include "utils/Logger.h"

#include <qjson/parser.h>
#include <qjson/serializer.h>

#include <QVariant>


class SipInfoPrivate : public QSharedData
{
public:
    SipInfoPrivate()
    {
    }

    SipInfoPrivate( const SipInfoPrivate& other ) : QSharedData( other ),
        nodeId(other.nodeId),
        key(other.key)
    {
    }
    ~SipInfoPrivate() { }

    QString nodeId;
    QString key;

    bool isValid( bool visible ) const
    {
        // visible and all data available
        if ( visible && !nodeId.isNull() && !key.isNull() )
            return true;
        // invisible and no data available
        if ( !visible && nodeId.isNull() && key.isNull() )
            return true;
        return false;
    }
};

class SipInfoHostPrivate : public QSharedData
{
public:
    SipInfoHostPrivate()
        : port( -1 )
    {
    }

    SipInfoHostPrivate( const SipInfoHostPrivate& other ) : QSharedData( other ),
        visible(other.visible),
        host(other.host),
        port(other.port)
    {
    }
    ~SipInfoHostPrivate() { }

    QVariant visible;
    QString host;
    int port;

    bool isValid() const
    {
        // visible and all data available
        if ( visible.toBool() && !host.isEmpty() && ( port > 0 ) )
            return true;
        // invisible and no data available
        if ( !visible.toBool() && host.isEmpty() && ( port < 0 ) )
            return true;
        return false;
    }
};


SipInfo::SipInfo()
{
    d = new SipInfoPrivate;
    d4 = new SipInfoHostPrivate;
    d6 = new SipInfoHostPrivate;
}


SipInfo::SipInfo(const SipInfo& other)
    : QObject()
    , d( other.d )
    , d4( other.d4 )
    , d6( other.d6 )
{
}


SipInfo::~SipInfo()
{
}


SipInfo&
SipInfo::operator=( const SipInfo& other )
{
    d = other.d;
    d4 = other.d4;
    d6 = other.d6;
    return *this;
}

void
SipInfo::clearHostPrivate(QSharedDataPointer< SipInfoHostPrivate > p)
{
    p->visible.clear();
    p->host = QString();
    p->port = -1;
}


void
SipInfo::clear()
{
    d->nodeId = QString();
    d->key = QString();
    clearHostPrivate(d4);
    clearHostPrivate(d6);
}


bool
SipInfo::isValid() const
{
    if( !d4->visible.isNull() && d4->isValid() && !d6->visible.isNull() && d6->isValid() )
        return d->isValid( d4->visible.toBool() || d6->visible.toBool() );
    return false;
}


void
SipInfo::setVisible( bool visible )
{
    d4->visible.setValue(visible);
}

void
SipInfo::setVisible6(bool visible)
{
    d6->visible.setValue(visible);
}


bool
SipInfo::isVisible() const
{
    Q_ASSERT( isValid() );

    return d4->visible.toBool();
}

bool
SipInfo::isVisible6() const
{
    Q_ASSERT( isValid() );

    return d6->visible.toBool();
}


void
SipInfo::setHost( const QString& host )
{
    d4->host = host;
}


const QString
SipInfo::host() const
{
    Q_ASSERT( isValid() );

    return d4->host;
}

void
SipInfo::setHost6( const QString& host )
{
    d6->host = host;
}


const QString
SipInfo::host6() const
{
    Q_ASSERT( isValid() );

    return d6->host;
}


void
SipInfo::setPort( int port )
{
    d4->port = port;
}


int
SipInfo::port() const
{
    Q_ASSERT( isValid() );

    return d4->port;
}

void
SipInfo::setPort6( int port )
{
    d6->port = port;
}


int
SipInfo::port6() const
{
    Q_ASSERT( isValid() );

    return d6->port;
}


void
SipInfo::setNodeId( const QString& nodeId )
{
    d->nodeId = nodeId;
}


const QString
SipInfo::nodeId() const
{
    Q_ASSERT( isValid() );

    return d->nodeId;
}


void
SipInfo::setKey( const QString& key )
{
    d->key = key;
}


const QString
SipInfo::key() const
{
    Q_ASSERT( isValid() );

    return d->key;
}


const QString
SipInfo::toJson() const
{
    // build variant map
    QVariantMap m;
    m["visible"] = isVisible();
    if( isVisible() )
    {
        m["ip"] = host();
        m["port"] = port();
    }
    m["visible6"] = isVisible6();
    if ( isVisible6() )
    {
        m["ip6"] = host6();
        m["port6"] = port6();
    }
    if ( isVisible() || isVisible6() )
    {
        m["key"] = key();
        m["uniqname"] = nodeId();
    }

    // serialize
    QJson::Serializer serializer;
    QByteArray ba = serializer.serialize( m );

    return QString::fromLatin1( ba );
}


const SipInfo
SipInfo::fromJson( QString json )
{
    SipInfo info;

    QJson::Parser parser;
    bool ok;
    QVariant v = parser.parse( json.toLatin1(), &ok );
    if ( !ok  || v.type() != QVariant::Map )
    {
        qDebug() << Q_FUNC_INFO << "Invalid JSON: " << json;
        return info;
    }
    QVariantMap m = v.toMap();

    info.setVisible( m["visible"].toBool() );
    // by default visible6 is false and does not need to be part of the JSON for backward compability
    if ( m.contains( "visible6" ) && m["visible6"].toBool() ) {
        info.setHost6( m["host6"].toString() );
        info.setPort6( m["port6"].toInt() );
    }
    else
        info.setVisible6( false );
    if ( m["visible"].toBool() )
    {
        info.setHost( m["host"].toString() );
        info.setPort( m["port"].toInt() );
    }
    if( m["visible"].toBool() || ( m.contains( "visible6" ) && m["visible6"].toBool() ) )
    {
        info.setNodeId( m["uniqname"].toString() );
        info.setKey( m["key"].toString() );
    }

    return info;
}


QDebug
operator<< ( QDebug dbg, const SipInfo& info )
{
    if( !info.isValid() )
        dbg.nospace() << "info is invalid";
    else
        dbg.nospace() << info.toJson();

    return dbg.maybeSpace();
}

bool operator==( const SipInfo& one, const SipInfo& two )
{
    // check valid/invalid combinations first, so we don't try to access any invalid sipInfos (->assert)
    if ( ( one.isValid() && !two.isValid() ) || ( !one.isValid() && two.isValid() ) )
    {
        return false;
    }
    else if ( one.isValid() && two.isValid() )
    {
        if ( one.isVisible() == two.isVisible()
            && one.host() == two.host()
            && one.port() == two.port()
            && one.isVisible6() == two.isVisible6()
            && one.host6() == two.host6()
            && one.port6() == two.port6()
            && one.nodeId() == two.nodeId()
            && one.key() == two.key() )
        {
            return true;
        }
    }

    return false;
}

const QString
SipInfo::debugString() const
{
    QString debugString( "SIP INFO: visible: %1 host: host %2 port: %3 nodeid: %4 key: %5 visible6: %7 host6: %8 port6: %9" );
    return debugString.arg( d4->visible.toBool() )
                      .arg( d4->host )
                      .arg( d4->port )
                      .arg( d->nodeId )
                      .arg( d->key )
                      .arg( d6->visible.toBool() )
                      .arg( d6->host )
                      .arg( d6->port );

}

