/**
 * NEMA23 quadrature mount.
 *
 * This catches the screw holes on the back of the NEMA23 motor using
 * long M3x30 screws and allows the quadrature encoder to be screwed
 * in with three M3x10 ones.
 *
 * Todo: channel for set screw, fix spacing on motor mounts.
 */
plate_thick = 15;
plate_side = 60;
$fs = 1;


module cross(thick)
{
for(i=[0,90])
{
	rotate(i)
	render() difference()
	{
		hull() {
			translate([0,+30,0])
			cylinder(r=10, h=thick);

			translate([0,-30,0])
			cylinder(r=10, h=thick);
		}

		translate([0,+30,-1]) cylinder(r=3.5/2, h=thick+2);
		translate([0,+30,4]) cylinder(r=10/2, h=thick+2);

		translate([0,-30,-1]) cylinder(r=3.5/2, h=thick+2);
		translate([0,-30,4]) cylinder(r=10/2, h=thick+2);
	}
}
}



render() difference()
{
	union()
	{
		cylinder(r=21, h=plate_thick);
		cross(10);
	}

	translate([0,0,-1]) cylinder(r=21/2, h=plate_thick+2);

	for(i=[0:2])
	{
		rotate(i*120) translate([15,0,-1]) {
			cylinder(r=3.5/2, h=plate_thick+2);
			cylinder(r=7/2, h=5);
		}
	}
}


//cross();

/*
translate([0,0,plate_thick/2])
cube([plate_side,plate_side,plate_thick], center=true);



render() difference()
{
	translate([0,0,plate_thick/2]) cube([plate_side,plate_side,plate_thick], center=true);
	translate([0,0,-1]) cylinder(d=20, h=plate_thick+2);

	for(i=[0:2])
	{
		rotate(i*120) translate([15,0,-1]) cylinder(d=3, h=plate_thick+2);
	}
}

module filet(r,h)
{
render() difference()
{
translate([0,0,h/2]) cube([r,r,h], center=true);
translate([r/2,r/2,0]) cylinder(d=r, h=h);
}
}

for(i=[0:3])
{
	rotate(i*90)
	translate([(plate_side-10)/2,(plate_side-10)/2,0])
	{
		difference()
		{
			translate([0,0,40/2]) cube([10,10,40], center=true);
			translate([-10/2,-10/2,0]) filet(10,50);
		}
	}
}
*/
