/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
// Implementation in UTIL.CPP
#ifndef SAVERESTORE_H
#define SAVERESTORE_H

class CSaveRestoreBuffer
{
public:
	CSaveRestoreBuffer( void );
	CSaveRestoreBuffer( SAVERESTOREDATA *pdata );
	~CSaveRestoreBuffer( void );

	int		EntityIndex( edict_t *pentLookup );
	int		EntityIndex( EOFFSET eoLookup );
	int		EntityFlags( int entityIndex, int flags ) { return EntityFlagsSet( entityIndex, 0 ); }
	int		EntityFlagsSet( int entityIndex, int flags );

	edict_t		*EntityFromIndex( int entityIndex );

	unsigned short	TokenHash( const char *pszToken );

protected:
	SAVERESTOREDATA	*m_pdata;
	void		BufferRewind( int size );
	unsigned int	HashString( const char *pszToken );
};


class CSave : public CSaveRestoreBuffer
{
public:
	CSave( SAVERESTOREDATA *pdata ) : CSaveRestoreBuffer( pdata ) {};

	void	WriteShort( const char *pname, const short *value, int count );
	void	WriteInt( const char *pname, const int *value, int count );		// Save an int
	void	WriteFloat( const char *pname, const float *value, int count );	// Save a float
	void	WriteTime( const char *pname, const float *value, int count );	// Save a float (timevalue)
	void	WriteData( const char *pname, int size, const char *pdata );		// Save a binary data block
	void	WriteString( const char *pname, const char *pstring );			// Save a null-terminated string
	void	WriteString( const char *pname, const int *stringId, int count );	// Save a null-terminated string (engine string)
	void	WriteVector( const char *pname, const Vector &value );				// Save a vector
	void	WriteVector( const char *pname, const float *value, int count );	// Save a vector
	void	WritePositionVector( const char *pname, const Vector &value );		// Offset for landmark if necessary
	void	WritePositionVector( const char *pname, const float *value, int count );	// array of pos vectors
	void	WriteFunction( const char *pname, const int *value, int count );		// Save a function pointer
	int	WriteFields( const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount );
	int	WriteProgFields( const char *pname, void *pBaseData, ddef_t *pFields, int fieldCount );
	int	WriteGlobalFields( const char *pname, void *pBaseData, ddef_t *pFields, int fieldCount );
private:
	int	DataEmpty( const char *pdata, int size );
	int	DataEmpty( const int *pdata, int size );
	void	BufferField( const char *pname, int size, const char *pdata );
	void	BufferString( char *pdata, int len );
	void	BufferData( const char *pdata, int size );
	void	BufferHeader( const char *pname, int size );
};

typedef struct 
{
	unsigned short	size;
	unsigned short	token;
	char		*pData;
} HEADER;

class CRestore : public CSaveRestoreBuffer
{
public:
	CRestore( SAVERESTOREDATA *pdata ) : CSaveRestoreBuffer( pdata ) { m_global = 0; m_precache = TRUE; }
	int	ReadFields( const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount );
	int	ReadProgFields( const char *pname, void *pBaseData, ddef_t *pFields, int fieldCount );
	int	ReadGlobalFields( const char *pname, void *pBaseData, ddef_t *pFields, int fieldCount );
	int	ReadField( void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount, int startField, int size, char *pName, void *pData );
	int	ReadField( void *pBaseData, ddef_t *pFields, int fieldCount, int startField, int size, char *pName, void *pData );
	int	ReadInt( void );
	short	ReadShort( void );
	int	ReadNamedInt( const char *pName );
	char	*ReadNamedString( const char *pName );
	int	Empty( void ) { return (m_pdata == NULL) || ((m_pdata->pCurrentData-m_pdata->pBaseData)>=m_pdata->bufferSize); }
	inline	void SetGlobalMode( int global ) { m_global = global; }
	void	PrecacheMode( BOOL mode ) { m_precache = mode; }
private:
	char	*BufferPointer( void );
	void	BufferReadBytes( char *pOutput, int size );
	void	BufferSkipBytes( int bytes );
	int	BufferSkipZString( void );
	int	BufferCheckZString( const char *string );

	void	BufferReadHeader( HEADER *pheader );

	int	m_global;		// Restoring a global entity?
	BOOL	m_precache;
};

#define MAX_ENTITYARRAY 64

#endif		//SAVERESTORE_H