// [ SETTINGS ] [ INDIRECT LIGHT ]

#define G_IL_QUALITY				2		// Quality of the IL [ 0 = Very Low | 1 = Low | 2 = Normal | 3 = High ]

#define G_IL_INTENSITY				8.0f	// Intensity of the indirect ilumination
#define G_IL_SKYLIGHT_INTENSITY		1.0f	// Intensity of the sky light
#define G_IL_COLOR_VIBRANCE			3.0f	// Vibrance of the indirect ilumination

#define G_IL_NOISE					0.25f	// Intensity of the noise applied to the IL

#define G_IL_RANGE 					1.0f	// Radius of the IL sampling [ Recommended 0.3f ~ 1.0f ]
#define G_IL_MAX_DIFFERENCE			20.0f	// Maximum distance the sampled color can travel

#define G_IL_WEAPON_LENGTH 			1.5f	// Maximum lenght of the weapon
#define G_IL_WEAPON_RANGE 			0.015f	// Weapons radius for the IL sampling

#define G_IL_DISCARD_SAMPLE_AT		0.2f	// If the intensity of the bounce is lower or equal to this value, discard the sample. ( 1.0f = 100% intensity )
											// The intensity of the bounce will vary depending on the angle of the surfaces and the distance.

//#define G_IL_DEBUG_MODE					// Uncomment if you want to check what the IL is doing