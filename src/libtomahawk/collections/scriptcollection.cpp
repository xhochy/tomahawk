#include "scriptcollection.h"


void
ScriptCollection::loadArtists()
{
    QList< Tomahawk::artist_ptr > artists;

    artists
        << Tomahawk::Artist::get("Knorkator", true)
        << Tomahawk::Artist::get("ScriptArtist 2", true)
        << Tomahawk::Artist::get("ScriptArtist 3", true);

    emit artistsLoaded( artists );
}


void
ScriptCollection::loadAlbums(const Tomahawk::artist_ptr& artist)
{
    QList< Tomahawk::album_ptr > albums;

    albums
        << Tomahawk::Album::get( artist, "ScriptAlbum 1" )
        << Tomahawk::Album::get( artist, "ScriptAlbum 2" );

    emit albumsLoaded( albums, artist );
}


void
ScriptCollection::loadTracks(const Tomahawk::album_ptr& album)
{
    QList< Tomahawk::result_ptr > tracks;

    emit tracksLoaded( tracks, album );
}
