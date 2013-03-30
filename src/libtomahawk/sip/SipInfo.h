/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
 *   Copyright 2011, Dominik Schmidt <dev@dominik-schmidt.de>
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

#ifndef SIPINFO_H
#define SIPINFO_H

#include <QSharedPointer>
#include <QHostInfo>

#include "DllMacro.h"

class SipInfoPrivate;
class SipInfoHostPrivate;

class DLLEXPORT SipInfo : public QObject
{
Q_OBJECT

public:
    SipInfo();
    SipInfo(const SipInfo& other);
    virtual ~SipInfo();
    SipInfo& operator=(const SipInfo& info);

    void clear();
    bool isValid() const;

    void setVisible( bool visible );
    bool isVisible() const;
    void setVisible6( bool visible );
    bool isVisible6() const;

    void setHost( const QString& host );
    const QString host() const;
    void setHost6( const QString& host );
    const QString host6() const;

    void setPort( int port );
    int port() const;
    void setPort6( int port );
    int port6() const;

    void setNodeId( const QString& nodeId );
    const QString nodeId() const;

    void setKey( const QString& key );
    const QString key() const;


    const QString toJson() const;
    static const SipInfo fromJson( QString json );

    const QString debugString() const;

private:
    QSharedDataPointer<SipInfoPrivate> d;
    QSharedDataPointer<SipInfoHostPrivate> d4;
    QSharedDataPointer<SipInfoHostPrivate> d6;
    static void clearHostPrivate(QSharedDataPointer<SipInfoHostPrivate> p);
};

DLLEXPORT QDebug operator<<( QDebug dbg, const SipInfo &info );
DLLEXPORT bool operator==( const SipInfo& one, const SipInfo& two );

#endif // SIPINFO_H
