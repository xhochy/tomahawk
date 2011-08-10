/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Dominik Schmidt <domme@tomahawk-player.org>
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

#ifndef DUMMYCOLLECTION_H
#define DUMMYCOLLECTION_H

#include "collection.h"
#include "typedefs.h"

#include "artist.h"
#include "album.h"
#include "track.h"


#include <QDir>

#include "dllmacro.h"

class DLLEXPORT DummyCollection : public Tomahawk::Collection
{
    Q_OBJECT

public:
    explicit DummyCollection( const Tomahawk::source_ptr& source, QObject* parent = 0 ) : Collection( source, "DOMME COLLECTION", parent ) {}
    ~DummyCollection()
    {
        qDebug() << Q_FUNC_INFO;
    }

    virtual void loadArtists();
    virtual void loadAlbums( const Tomahawk::artist_ptr& artist );
    virtual void loadTracks( const Tomahawk::album_ptr& album );

    virtual void addTracks ( const QList< QVariant >& newitems ) {}
    virtual void removeTracks ( const QDir& dir ) {}
};

#endif // DUMMYCOLLECTION_H
