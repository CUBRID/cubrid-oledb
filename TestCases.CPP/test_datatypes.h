typedef struct
{
	short yr;
	short mon;
	short day;
} CUBRIDDate;

typedef struct
{
	short yr;
	short mon;
	short day;
	short hh;
	short mm;
	short ss;
	long ms;
} CUBRIDDateTime;

typedef struct
{
	short hh;
	short mm;
	short ss;
} CUBRIDTime;

typedef struct
{
	short yr;
	short mon;
	short day;
	short hh;
	short mm;
	short ss;
} CUBRIDTimestamp;

typedef struct
{
	BYTE precision;
	BYTE scale;
	BYTE sign;
	BYTE val[16];
} CUBRIDNumeric;


bool Test_Dynamic_DataTypes_SELECT();
bool Test_Dynamic_DataTypes_INSERT();
bool Test_DataTypes_INSERT();

