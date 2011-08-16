#include "scriptcollection.h"


ScriptCollection::ScriptCollection(const Tomahawk::source_ptr& source, QtScriptResolver* resolver, QObject* parent): Collection(source, "NAME NOT FOUND", parent)
{
    Q_ASSERT( resolver != 0 );
    m_resolver = resolver;
    QVariantMap settings = m_resolver->resolverSettings();
    setName( settings.value("name").toString() );
}


void
ScriptCollection::loadArtists()
{
    QVariant results = m_resolver->runJS( "Tomahawk.resolver.instance.getArtists();" );

    QList< Tomahawk::artist_ptr > artists;
    foreach( const QVariant& result, results.toList() )
    {
        artists << Tomahawk::Artist::get(result.toString(), true);
    }

    emit artistsLoaded( artists );
}


void
ScriptCollection::loadAlbums(const Tomahawk::artist_ptr& artist)
{
    QVariant results = m_resolver->runJS(
        QString("Tomahawk.resolver.instance.getAlbums(\"%1\");").arg( artist->name() )
    );

    qDebug() << results;

    QList< Tomahawk::album_ptr > albums;
    foreach( const QVariant& result, results.toList() )
    {
        qDebug() << result.toString();
        albums << Tomahawk::Album::get( artist, result.toString(), true );
    }

    emit albumsLoaded( albums, artist );
}


void
ScriptCollection::loadTracks(const Tomahawk::album_ptr& album)
{
    QVariant results = m_resolver->runJS(
        QString("Tomahawk.resolver.instance.getTracks(\"%1\", \"%2\");")
            .arg( album->artist()->name() )
            .arg( album->name() )
    );

    QList< Tomahawk::result_ptr > tracks = m_resolver->parseResultVariantList( results.toMap().value("results").toList() );

    emit tracksLoaded( tracks, album );
}
