#pragma once

#include "API.h"

#include "Platform/Locks.h"
#include "Platform/Thread.h"

#include "Foundation/Log.h"
#include "Foundation/String.h"
#include "Foundation/ReferenceCounting.h"

#include "Reflect/Object.h"
#include "Reflect/TranslatorDeduction.h"

#include "Persist/ArchiveBson.h"

#include <mongo-c/src/mongo.h>

#if HELIUM_CPP11
# include <functional>
#endif

#define HELIUM_MONGO_DEFAULT_PORT ( 27017 )

namespace Helium
{
	namespace Mongo
	{
		HELIUM_MONGO_API void        Initialize();
		HELIUM_MONGO_API void        Cleanup();
		HELIUM_MONGO_API const char* GetErrorString( int status );

		class HELIUM_MONGO_API Model : public Helium::Reflect::Object
		{
		public:
			// determines which collection in the database to use by default, declare a copy of this in derived
			//  classes to set the default collection (this one is NULL which will cause the class name to be used)
			static const char* defaultCollection;

			// the unique-id of the model instance, generated by the client just before insert
			//  you should not insert an object twice, instead null out the id before inserting another copy
			Helium::Persist::BsonObjectId id;

			HELIUM_DECLARE_CLASS( Model, Helium::Reflect::Object );
			static void PopulateMetaType( Helium::Reflect::MetaStruct& type );
		};

		class Database;

		template< class T >
		class HELIUM_MONGO_API Cursor : public Helium::NonCopyable
		{
		public:
			Cursor( int options = 0x0 );
			~Cursor();

			// set the state of the cursor (used by the database class)
			void Set( Database* db, mongo_cursor* cursor );

			// get a bunch of result objects from the cursor
			bool Get( Helium::DynamicArray< Helium::StrongPtr< T > >& objects, size_t maxCount = ~0 );

			// get a single result object from the cursor
			Helium::StrongPtr< T > Next();

#if HELIUM_CPP11
			// process each result object
			void Process( std::function< void ( T* ) > function );
#endif

			inline int  GetOptions();
			inline void SetOptions( int options );

		private:
			Database*     db;
			mongo_cursor* cursor;
			int           options; // options flags are defined in enum mongo_cursor_opts
		};

		class HELIUM_MONGO_API Database : public Helium::NonCopyable
		{
		public:
			template< class >
			friend class Cursor;

			Database( const char* name = "" );
			~Database();

			// db ops/preferences
			void SetName( const char* name );
			void SetTimeout( int timeoutMilliseconds );
			bool Connect( const char* addr, uint16_t port = HELIUM_MONGO_DEFAULT_PORT );
			inline bool IsConnected() const;
			inline void SetThread( Helium::Thread::id_t threadId = Helium::GetCurrentThreadID() );
			double GetCollectionCount( const char* name );
			bool CreateCappedCollection( const char* name, int cappedSizeInBytes, int cappedMaxCount = 0 );

			// single insert
			template< class T >
			bool Insert( const Helium::StrongPtr< T >& object );
			bool Insert( const Helium::StrongPtr< Model >& object, const char* collection = NULL );

			// single update
			template< class T >
			bool Update( const Helium::StrongPtr< T >& object );
			bool Update( const Helium::StrongPtr< Model >& object, const char* collection = NULL );

			// batch insert, default collection will be named for the type of collection objects
			template< class T >
			bool Insert( Helium::StrongPtr< T >* objects, size_t count );
			bool Insert( Helium::StrongPtr< Model >* objects, size_t count, const char* collection = NULL );

			// single fetch
			template< class T >
			bool Get( const Helium::StrongPtr< T >& object );
			bool Get( const Helium::StrongPtr< Model >& object, const char* collection = NULL );

			// find
			//  query == NULL will return all objects by default
			//  fields == NULL will populate all fields in result objects, populate to determine which fields are updated
			//  collection == NULL uses collection named for the type specified in the cursor object
			template< class T >
			bool Find( Cursor< T >& result, const bson* query = NULL, const bson* fields = NULL, const char* collection = NULL, int limit = 0, int skip = 0 );

		private:
			Helium::String       name;
			bool                 isConnected;
			mongo                conn[1];
			Helium::Thread::id_t threadId;
		};
	}
}

#include "Mongo/Mongo.inl"