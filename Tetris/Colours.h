struct Colour
{
	u8 r;
	u8 g;
	u8 b;
	u8 a;
};

inline Colour
colour(u8 r, u8 g, u8 b, u8 a)
{
	return Colour{ r, g, b, a, };
}

const Colour BASIC_COLOURS[] = {
	colour(0x28, 0x28, 0x28, 0xFF),
	colour(0, 206, 209, 0xFF),
	colour(255, 193, 7, 0xFF),
	colour(128, 0, 128, 0xFF),
	colour(40, 167, 69, 0xFF),
	colour(238, 24, 72, 0xFF),
	colour(0, 0, 255, 0xFF),
	colour(230, 102, 70, 0xFF)
};

const Colour SMOOTH_COLOURS[] = {
	colour(0x28, 0x28, 0x28, 0xFF),
	colour(64, 224, 208, 0xFF),
	colour(222, 221, 170, 0xFF),
	colour(207, 12, 205, 0xFF),
	colour(129, 190, 139, 0xFF),
	colour(255, 115, 115, 0xFF),
	colour(70, 132, 153, 0xFF),
	colour(235, 158, 128, 0xFF),
};

const Colour DARK_COLOURS[] = {
	colour(0x28, 0x28, 0x28, 0xFF),
	colour(47, 79, 69, 0xFF),
	colour(153, 127, 40, 0xFF),
	colour(71, 26, 128, 0xFF),
	colour(6, 85, 53, 0xFF),
	colour(130, 9, 1, 0xFF),
	colour(14, 47, 68, 0xFF),
	colour(209, 88, 55, 0xFF),
};