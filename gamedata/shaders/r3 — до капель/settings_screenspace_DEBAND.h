// [ SETTINGS ] [ SKY DEBANDING ]
#ifdef MAX_QUALITY_SHADERS
#define G_DEBANDING_QUALITY		2	// Deband iterations [ 1 = Low, 2 = Normal, 3 = High, 4 = Ultra ]
#else
#define G_DEBANDING_QUALITY		0	// Deband iterations [ 1 = Low, 2 = Normal, 3 = High, 4 = Ultra ]	
#endif
#define G_DEBANDING_RADIUS		48	// Max pixels to search for banding. Higher values improve smoothness, but also add more noise in some circumstances.
