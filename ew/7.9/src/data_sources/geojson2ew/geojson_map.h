typedef struct _GEOJSON_MAP_ENTRY {
	char *path;
	char channelCode;
	double multiplier;
	char *conditionPath;
        struct _GEOJSON_MAP_ENTRY *next;
} GEOJSON_MAP_ENTRY;

int add_channel(GEOJSON_MAP_ENTRY *);
GEOJSON_MAP_ENTRY *get_channel(); 
