#include "dummycollection.h"


void
DummyCollection::loadArtists()
{
    QList< Tomahawk::artist_ptr > artists;

    artists
        << Tomahawk::Artist::get("Iron Maiden")
        << Tomahawk::Artist::get("Saxon")
        << Tomahawk::Artist::get("Knorkator");

    emit artistsLoaded( artists );
}


void
DummyCollection::loadAlbums(const Tomahawk::artist_ptr& artist)
{
    QList< Tomahawk::album_ptr > albums;

    albums
        << Tomahawk::Album::get( artist, "Test 1" )
        << Tomahawk::Album::get( artist, "Test 2" );

    emit albumsLoaded( albums, artist );
}


void
DummyCollection::loadTracks(const Tomahawk::album_ptr& album)
{
    QList< Tomahawk::result_ptr > tracks;

    emit tracksLoaded( tracks, album );
}
