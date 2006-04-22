/* $Id$ */

typedef struct CargoTypesValues {
	StringID names[NUM_CARGO];
	StringID units_volume[NUM_CARGO];
	byte weights[NUM_CARGO];
	SpriteID sprites[NUM_CARGO];

	uint16 initial_cargo_payment[NUM_CARGO];
	byte transit_days_table_1[NUM_CARGO];
	byte transit_days_table_2[NUM_CARGO];

	byte road_veh_by_cargo_start[NUM_CARGO];
	byte road_veh_by_cargo_count[NUM_CARGO];
} CargoTypesValues;


static const CargoTypesValues _cargo_types_base_values[4] = {
	{
		/* normal names */
		{
			STR_000F_PASSENGERS,
			STR_0010_COAL,
			STR_0011_MAIL,
			STR_0012_OIL,
			STR_0013_LIVESTOCK,
			STR_0014_GOODS,
			STR_0015_GRAIN,
			STR_0016_WOOD,
			STR_0017_IRON_ORE,
			STR_0018_STEEL,
			STR_0019_VALUABLES,
			STR_000E,
		},

		{ /* normal units of volume */
			STR_PASSENGERS,
			STR_TONS,
			STR_BAGS,
			STR_LITERS,
			STR_ITEMS,
			STR_CRATES,
			STR_TONS,
			STR_TONS,
			STR_TONS,
			STR_TONS,
			STR_BAGS,
			STR_RES_OTHER
		},

		/* normal weights */
		{
			1, 16, 4, 16, 3, 8, 16, 16, 16, 16, 2, 0,
		},

		/* normal sprites */
		{
			0x10C9, 0x10CA, 0x10CB, 0x10CC, 0x10CD, 0x10CE, 0x10CF, 0x10D0, 0x10D1, 0x10D2, 0x10D3,
		},

		/* normal initial cargo payment */
		{
			3185, 5916, 4550, 4437, 4322, 6144, 4778, 5005, 5120, 5688, 7509, 5688
		},

		/* normal transit days table 1 */
		{
			0, 7, 20, 25, 4, 5, 4, 15, 9, 7, 1, 0,
		},

		/* normal transit days table 2 */
		{
			24, 255, 90, 255, 18, 28, 40, 255, 255, 255, 32, 30,
		},

		/* normal road veh by cargo start & count */
		{116, 123, 126, 132, 135, 138, 141, 144, 147, 150, 153, 156},
		{7, 3, 6, 3, 3, 3, 3, 3, 3, 3, 3, 3}
	},

	{
		/* hilly names */
		{
			STR_000F_PASSENGERS,
			STR_0010_COAL,
			STR_0011_MAIL,
			STR_0012_OIL,
			STR_0013_LIVESTOCK,
			STR_0014_GOODS,
			STR_0022_WHEAT,
			STR_0016_WOOD,
			STR_000E,
			STR_001F_PAPER,
			STR_0020_GOLD,
			STR_001E_FOOD,
		},

		{ /* hilly units of volume */
			STR_PASSENGERS,
			STR_TONS,
			STR_BAGS,
			STR_LITERS,
			STR_ITEMS,
			STR_CRATES,
			STR_TONS,
			STR_TONS,
			STR_RES_OTHER,
			STR_TONS,
			STR_BAGS,
			STR_TONS
		},

		/* hilly weights */
		{
			1, 16, 4, 16, 3, 8, 16, 16, 0, 16, 8, 16
		},

		/* hilly sprites */
		{
			0x10C9, 0x10CA, 0x10CB, 0x10CC, 0x10CD, 0x10CE, 0x10CF, 0x10D0, 0x2, 0x10D9, 0x10D3, 0x10D8
		},

		/* hilly initial cargo payment */
		{
			3185, 5916, 4550, 4437, 4322, 6144, 4778, 5005, 5120, 5461, 5802, 5688
		},

		/* hilly transit days table 1 */
		{
			0, 7, 20, 25, 4, 5, 4, 15, 9, 7, 10, 0,
		},

		/* hilly transit days table 2 */
		{
			24, 255, 90, 255, 18, 28, 40, 255, 255, 60, 40, 30
		},

		/* hilly road veh by cargo start & count */
		{116, 123, 126, 132, 135, 138, 141, 144, 147, 159, 153, 156},
		{7, 3, 6, 3, 3, 3, 3, 3, 3, 3, 3, 3},
	},

	{
		/* desert names */
		{
			STR_000F_PASSENGERS,
			STR_0023_RUBBER,
			STR_0011_MAIL,
			STR_0012_OIL,
			STR_001C_FRUIT,
			STR_0014_GOODS,
			STR_001B_MAIZE,
			STR_0016_WOOD,
			STR_001A_COPPER_ORE,
			STR_0021_WATER,
			STR_001D_DIAMONDS,
			STR_001E_FOOD
		},

		{ /* desert units of volume */
			STR_PASSENGERS,
			STR_LITERS,
			STR_BAGS,
			STR_LITERS,
			STR_TONS,
			STR_CRATES,
			STR_TONS,
			STR_TONS,
			STR_TONS,
			STR_LITERS,
			STR_BAGS,
			STR_TONS
		},

		/* desert weights */
		{
			1, 16, 4, 16, 16, 8, 16, 16, 16, 16, 2, 16,
		},

		/* desert sprites */
		{
			0x10C9, 0x10DA, 0x10CB, 0x10CC, 0x10D4, 0x10CE, 0x10CF, 0x10D0, 0x10D5, 0x10D6, 0x10D7, 0x10D8
		},

		/* desert initial cargo payment */
		{
			3185, 4437, 4550, 4892, 4209, 6144, 4322, 7964, 4892, 4664, 5802, 5688
		},

		/* desert transit days table 1 */
		{
			0, 2, 20, 25, 0, 5, 4, 15, 12, 20, 10, 0
		},

		/* desert transit days table 2 */
		{
			24, 20, 90, 255, 15, 28, 40, 255, 255, 80, 255, 30
		},

		/* desert road veh by cargo start & count */
		{116, 171, 126, 132, 168, 138, 141, 144, 162, 165, 153, 156},
		{7, 3, 6, 3, 3, 3, 3, 3, 3, 3, 3, 3}
	},

	{
		/* candy names */
		{
			STR_000F_PASSENGERS,
			STR_0024_SUGAR,
			STR_0011_MAIL,
			STR_0025_TOYS,
			STR_002B_BATTERIES,
			STR_0026_CANDY,
			STR_002A_TOFFEE,
			STR_0027_COLA,
			STR_0028_COTTON_CANDY,
			STR_0029_BUBBLES,
			STR_002C_PLASTIC,
			STR_002D_FIZZY_DRINKS,
		},

		{ /* candy unitrs of volume */
			STR_PASSENGERS,
			STR_TONS,
			STR_BAGS,
			STR_NOTHING,
			STR_NOTHING,
			STR_TONS,
			STR_TONS,
			STR_LITERS,
			STR_TONS,
			STR_NOTHING,
			STR_LITERS,
			STR_NOTHING
		},

		/* candy weights */
		{
			1, 16, 4, 2, 4, 5, 16, 16, 16, 1, 16, 2
		},

		/* candy sprites */
		{
			0x10C9, 0x10DC, 0x10CB, 0x10DD, 0x10E3, 0x10DB, 0x10E0, 0x10D6, 0x10DE, 0x10E1, 0x10E2, 0x10DF
		},

		/* candy initial cargo payment */
		{
			3185, 4437, 4550, 5574, 4322, 6144, 4778, 4892, 5005, 5077, 4664, 6250
		},

		/* candy transit days table 1 */
		{
			0, 20, 20, 25, 2, 8, 14, 5, 10, 20, 30, 30,
		},

		/* candy transit days table 2 */
		{
			24, 255, 90, 255, 30, 40, 60, 75, 25, 80, 255, 50
		},

		/* candy road veh by cargo start & count */
		{116, 174, 126, 186, 192, 189, 183, 177, 180, 201, 198, 195},
		{7, 3, 6, 3, 3, 3, 3, 3, 3, 3, 3, 3}
	}
};
