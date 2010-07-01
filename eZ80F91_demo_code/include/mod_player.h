// Standard mod data imports
extern void mod_data_reset( void );
extern void mod_data_read( void* dest, u24 nof_bytes );

// Max number of patterns to allow, requires 1KB RAM per pattern
#define MOD_MAX_PATTERNS 128

// File read callback
typedef void ( *fp_read )( void* pDest, u24 nof_bytes );

// Play module
extern void play_module( void );

// Load a module into memory
extern void load_module( fp_read read_data );
