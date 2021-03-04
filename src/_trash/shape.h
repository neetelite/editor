struct Line3
{
	/* A segment of a line */

	v3 start;
	v3 end;

	v3 dir;
	f32 dist;

	bool has_start;
	bool has_end;
};

struct Rec2
{
	union
	{
		struct
		{
			v2 start;
			v2 end;
		};
		v4 vec;
	};
};

struct Box2
{
	v2 pos;
	v2 dim;
	f32 rot;
	v2 scl; /* Premultiplied */
};

struct Rec3
{
	v3 start;
	v3 end;
};

struct Box3
{
	v3 pos;
	v3 dim;
	v3 rot;
	v3 scl; /* Premultiplied */
};

struct Box3 BOX3(v3, v3, v3);
