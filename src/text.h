enum AlignHorizontal
{
	align_horizontal_left,
	align_horizontal_middle,
	align_horizontal_right,
};

enum AlignVertical
{
	align_vertical_top,
	align_vertical_center,
	align_vertical_bottom,
};

struct Alignment
{
	enum AlignHorizontal horizontal;
	enum AlignVertical vertical;
};

struct Alignment
ALIGN(CString horizontal, CString vertical)
{
	struct Alignment result = {0};

	if(0) {}
	else if(cstr_eql(horizontal, "left")) result.horizontal = align_horizontal_left;
	else if(cstr_eql(horizontal, "middle")) result.horizontal = align_horizontal_middle;
	else if(cstr_eql(horizontal, "right")) result.horizontal = align_horizontal_right;
	else DBG_ERROR("Bad alignment\n");

	if(0) {}
	else if(cstr_eql(vertical, "top")) result.vertical = align_vertical_top;
	else if(cstr_eql(vertical, "center")) result.vertical = align_vertical_center;
	else if(cstr_eql(vertical, "bottom")) result.vertical = align_vertical_bottom;
	else DBG_ERROR("Bad alignment\n");

	return(result);
}
