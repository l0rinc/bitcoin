// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <bench/bench.h>
#include <random.h>
#include <span.h>
#include <streams.h>

#include <cstddef>
#include <vector>
#include <bench/bench.h>
#include <random.h>
#include <span.h>
#include <streams.h>

#include <cstddef>
#include <vector>
#include <map>
#include <cmath>

static void XorHistogram(benchmark::Bench& bench)
{
    // The histogram represents util::Xor method's write.size() histograms for the first 400k blocks, aggregated using:
    // awk '{ count[$1]++ } END { for (val in count) { printf "{%s, %.0f},\n", val, count[val] } }' histogram.txt
    const std::map<size_t, uint64_t> raw_histogram{
        {1, 3437372259},
        {2, 5351966},
        {3, 3090},
        {4, 1426386483},
        {5, 663},
        {6, 1157},
        {7, 120420},
        {8, 581705294},
        {9, 20642},
        {10, 31342},
        {11, 108794},
        {12, 66638},
        {13, 61955},
        {14, 25416},
        {15, 43721},
        {16, 23699},
        {17, 32994},
        {18, 8814},
        {19, 11762},
        {20, 8461},
        {21, 260608546},
        {22, 9558},
        {23, 12217127},
        {24, 1757},
        {25, 564661594},
        {26, 9292},
        {27, 2061},
        {28, 4227},
        {29, 1152},
        {30, 86840},
        {31, 1894},
        {32, 517717173},
        {33, 957936},
        {34, 29814},
        {35, 214262},
        {36, 46661},
        {37, 27151},
        {38, 12270},
        {39, 86014},
        {40, 15699},
        {41, 21769},
        {42, 113939},
        {43, 5471},
        {44, 4787},
        {45, 658},
        {46, 1486},
        {47, 87},
        {48, 984},
        {49, 522},
        {50, 568},
        {51, 657},
        {52, 1912},
        {53, 14132},
        {54, 2997},
        {55, 5331},
        {56, 2378},
        {57, 636},
        {58, 433},
        {59, 4593},
        {60, 4887},
        {61, 4000},
        {62, 1346},
        {63, 2194},
        {64, 2032},
        {65, 1672},
        {66, 1660},
        {67, 1737146},
        {68, 1530},
        {69, 1211},
        {70, 6050},
        {71, 462683},
        {72, 597471},
        {73, 888949},
        {74, 467340},
        {75, 11514},
        {76, 3914},
        {77, 787},
        {78, 896},
        {79, 1068},
        {80, 5917},
        {81, 3945},
        {82, 12052},
        {83, 42910},
        {84, 4009},
        {85, 2413},
        {86, 10136},
        {87, 2962},
        {88, 665343},
        {89, 3263},
        {90, 3581},
        {91, 3017},
        {92, 2713},
        {93, 1824},
        {94, 27468},
        {95, 3922211},
        {96, 42739},
        {97, 165407},
        {98, 93081},
        {99, 70137},
        {100, 11497},
        {103, 59795},
        {104, 21202},
        {105, 1446445},
        {106, 128915060},
        {107, 148677198},
        {108, 22941112},
        {109, 631},
        {110, 4740},
        {111, 120677},
        {112, 118593},
        {113, 1712},
        {114, 6},
        {115, 6},
        {116, 9},
        {117, 9},
        {118, 2},
        {122, 8},
        {123, 12},
        {125, 10},
        {126, 4536},
        {127, 1097686},
        {128, 2},
        {131, 6},
        {132, 6},
        {133, 7},
        {134, 2},
        {135, 447},
        {136, 3080},
        {137, 568380},
        {138, 61547986},
        {139, 97307078},
        {140, 37510818},
        {141, 492},
        {142, 2916},
        {143, 5487},
        {144, 2551},
        {145, 4173},
        {146, 5795},
        {147, 1489},
        {148, 116},
        {149, 19},
        {151, 6},
        {154, 10},
        {155, 1},
        {156, 6},
        {157, 8},
        {160, 2},
        {161, 6},
        {162, 4},
        {165, 5},
        {167, 131},
        {168, 134},
        {169, 109},
        {170, 3},
        {171, 2},
        {173, 5},
        {174, 2},
        {178, 30},
        {179, 24},
        {180, 814},
        {181, 885},
        {182, 19},
        {185, 4},
        {186, 140},
        {187, 79},
        {188, 50},
        {189, 14},
        {190, 4},
        {191, 2},
        {193, 2},
        {195, 3},
        {201, 33549},
        {202, 5},
        {205, 14},
        {208, 4},
        {209, 10},
        {210, 102},
        {211, 161},
        {212, 66},
        {214, 22},
        {215, 108},
        {216, 9073},
        {217, 572193},
        {218, 1175905},
        {219, 685958},
        {220, 93131},
        {221, 21184},
        {222, 2},
        {226, 2},
        {227, 24},
        {228, 16},
        {229, 12},
        {230, 2},
        {232, 2},
        {235, 2},
        {239, 2},
        {240, 2},
        {241, 354},
        {242, 668},
        {243, 336},
        {244, 19},
        {245, 40},
        {246, 19},
        {248, 6},
        {249, 35},
        {250, 1214},
        {251, 23496},
        {252, 1373022},
        {253, 2799235},
        {254, 1569384},
        {255, 140649},
        {256, 20560},
        {257, 26},
        {258, 4},
        {265, 2},
        {266, 2},
        {267, 2},
        {276, 6},
        {277, 8},
        {278, 6},
        {281, 11},
        {282, 272},
        {283, 1003},
        {284, 2108},
        {285, 3397},
        {286, 22746},
        {287, 40510},
        {288, 19969},
        {289, 133},
        {290, 44},
        {292, 2},
        {293, 2},
        {294, 4},
        {295, 2},
        {297, 1},
        {298, 2},
        {301, 2},
        {302, 6},
        {305, 2},
        {306, 2},
        {307, 2},
        {309, 6},
        {311, 2},
        {312, 2},
        {313, 2},
        {314, 28},
        {315, 56},
        {316, 1286},
        {317, 3257},
        {318, 3110},
        {319, 10960},
        {320, 19012},
        {321, 10378},
        {322, 3199},
        {323, 4466},
        {324, 2414},
        {325, 604},
        {326, 1001},
        {327, 819},
        {328, 552},
        {329, 202},
        {330, 39},
        {331, 8},
        {332, 10},
        {333, 5},
        {334, 10},
        {335, 16},
        {336, 2},
        {343, 749},
        {344, 1524},
        {345, 752},
        {347, 71},
        {348, 5673},
        {349, 15288},
        {350, 15691},
        {351, 8231},
        {352, 1963},
        {353, 6},
        {354, 6},
        {355, 9},
        {356, 2},
        {357, 68},
        {358, 3752},
        {359, 11551},
        {360, 15424},
        {361, 9973},
        {362, 4778},
        {363, 1668},
        {364, 281},
        {365, 6},
        {366, 2},
        {367, 2},
        {368, 14},
        {369, 60},
        {370, 32},
        {371, 14},
        {373, 2},
        {380, 2},
        {382, 2},
        {383, 4},
        {384, 2},
        {385, 6},
        {386, 2},
        {387, 4},
        {388, 8},
        {389, 6},
        {390, 10},
        {391, 74},
        {392, 3022},
        {393, 8343},
        {394, 8377},
        {395, 2914},
        {396, 144},
        {397, 36},
        {398, 14},
        {399, 6},
        {400, 16},
        {401, 24},
        {402, 14},
        {403, 14},
        {404, 12},
        {405, 18},
        {406, 12},
        {408, 2},
        {409, 4},
        {411, 2},
        {414, 6},
        {415, 252},
        {416, 1082},
        {417, 1623},
        {418, 1019},
        {419, 277},
        {420, 32},
        {421, 81},
        {422, 129},
        {423, 94},
        {424, 51},
        {425, 27},
        {426, 37},
        {427, 86},
        {428, 74},
        {429, 30},
        {430, 8},
        {431, 14},
        {432, 22},
        {433, 12},
        {434, 11},
        {436, 2},
        {438, 6},
        {439, 2},
        {440, 4},
        {453, 2},
        {454, 2},
        {456, 2},
        {458, 2},
        {460, 2},
        {461, 2},
        {462, 25},
        {463, 106},
        {464, 264},
        {465, 541},
        {466, 691},
        {467, 500},
        {468, 105},
        {470, 4},
        {471, 2},
        {472, 2},
        {473, 6},
        {474, 2},
        {475, 4},
        {476, 2},
        {477, 2},
        {481, 5},
        {482, 8},
        {483, 16},
        {484, 8},
        {485, 2},
        {486, 5},
        {487, 491},
        {488, 1405},
        {489, 1810},
        {490, 1317},
        {491, 830},
        {492, 343},
        {493, 60},
        {495, 2},
        {496, 2},
        {497, 2},
        {498, 12},
        {499, 24},
        {500, 26},
        {501, 12},
        {504, 2},
        {511, 2},
        {513, 2},
        {515, 2},
        {516, 2},
        {517, 2},
        {522, 2},
        {523, 4},
        {524, 2},
        {525, 4},
        {526, 4},
        {529, 4},
        {530, 4},
        {531, 2},
        {532, 2},
        {534, 2},
        {536, 4},
        {538, 2},
        {540, 8},
        {541, 12},
        {542, 8},
        {543, 2},
        {544, 4},
        {545, 2},
        {546, 2},
        {548, 2},
        {549, 2},
        {551, 2},
        {553, 30},
        {554, 158},
        {555, 257},
        {556, 352},
        {557, 240},
        {558, 110},
        {559, 16},
        {561, 16},
        {562, 10},
        {563, 10},
        {564, 14},
        {565, 8},
        {566, 2},
        {568, 4},
        {569, 2},
        {570, 2},
        {577, 2},
        {581, 4},
        {583, 6},
        {584, 60},
        {585, 58},
        {587, 4},
        {588, 2},
        {589, 4},
        {590, 2},
        {591, 2},
        {592, 4},
        {595, 2},
        {597, 2},
        {598, 4},
        {601, 2},
        {605, 2},
        {609, 2},
        {610, 2},
        {613, 2},
        {614, 6},
        {617, 2},
        {619, 2},
        {620, 2},
        {621, 6},
        {622, 10},
        {623, 10},
        {625, 2},
        {626, 2},
        {627, 6},
        {628, 6},
        {629, 4},
        {630, 4},
        {631, 4},
        {632, 2},
        {636, 4},
        {638, 2},
        {639, 2},
        {640, 4},
        {641, 6},
        {642, 6},
        {643, 2},
        {644, 8},
        {645, 4},
        {646, 10},
        {647, 2},
        {650, 2},
        {651, 4},
        {652, 6},
        {653, 4},
        {654, 2},
        {655, 2},
        {657, 2},
        {660, 2},
        {673, 2},
        {677, 4},
        {679, 4},
        {680, 2},
        {686, 6},
        {687, 2},
        {688, 2},
        {689, 4},
        {690, 2},
        {691, 2},
        {692, 4},
        {693, 4},
        {694, 8},
        {695, 2},
        {697, 2},
        {699, 9},
        {700, 6},
        {701, 12},
        {702, 12},
        {703, 14},
        {704, 6},
        {705, 4},
        {708, 2},
        {709, 2},
        {710, 2},
        {711, 2},
        {712, 4},
        {714, 4},
        {715, 2},
        {717, 2},
        {718, 2},
        {719, 2},
        {721, 4},
        {724, 4},
        {725, 2},
        {727, 2},
        {728, 2},
        {730, 4},
        {731, 2},
        {733, 4},
        {735, 2},
        {736, 2},
        {739, 2},
        {740, 2},
        {743, 4},
        {745, 2},
        {749, 2},
        {751, 2},
        {752, 2},
        {753, 2},
        {755, 6},
        {756, 2},
        {758, 7},
        {759, 6},
        {760, 20},
        {761, 28},
        {762, 10},
        {763, 10},
        {764, 2},
        {765, 4},
        {766, 4},
        {767, 4},
        {768, 6},
        {769, 4},
        {771, 2},
        {772, 2},
        {773, 2},
        {774, 6},
        {782, 2},
        {783, 2},
        {787, 2},
        {789, 4},
        {792, 2},
        {794, 4},
        {796, 2},
        {797, 4},
        {802, 8},
        {804, 2},
        {807, 26},
        {808, 4},
        {811, 2},
        {812, 2},
        {813, 4},
        {814, 6},
        {815, 4},
        {816, 2},
        {821, 2},
        {823, 4},
        {825, 2},
        {826, 2},
        {827, 2},
        {829, 2},
        {830, 8},
        {831, 7},
        {832, 6},
        {833, 6},
        {834, 2},
        {835, 2},
        {836, 2},
        {838, 4},
        {841, 2},
        {843, 4},
        {847, 8},
        {852, 2},
        {853, 2},
        {855, 2},
        {857, 4},
        {858, 10},
        {859, 20},
        {860, 8},
        {861, 10},
        {862, 10},
        {863, 4},
        {865, 2},
        {866, 2},
        {867, 4},
        {871, 2},
        {874, 2},
        {875, 4},
        {876, 2},
        {877, 2},
        {879, 2},
        {883, 2},
        {884, 2},
        {889, 2},
        {893, 4},
        {895, 6},
        {896, 2},
        {900, 2},
        {902, 2},
        {905, 4},
        {906, 2},
        {907, 2},
        {916, 6},
        {917, 4},
        {922, 4},
        {924, 2},
        {926, 6},
        {928, 2},
        {929, 2},
        {935, 4},
        {942, 4},
        {943, 2},
        {945, 2},
        {946, 2},
        {947, 2},
        {952, 6},
        {953, 2},
        {954, 2},
        {960, 2},
        {961, 2},
        {962, 2},
        {963, 4},
        {965, 2},
        {967, 2},
        {968, 4},
        {969, 2},
        {970, 2},
        {974, 2},
        {976, 10},
        {977, 13},
        {978, 4},
        {979, 8},
        {980, 8},
        {981, 8},
        {982, 2},
        {983, 2},
        {987, 4},
        {991, 4},
        {993, 2},
        {995, 6},
        {999, 2},
        {1000, 6},
        {1001, 4},
        {1005, 4},
        {1006, 2},
        {1007, 2},
        {1009, 2},
        {1010, 14},
        {1011, 10},
        {1012, 2},
        {1013, 2},
        {1016, 4},
        {1017, 2},
        {1018, 2},
        {1020, 4},
        {1022, 2},
        {1023, 2},
        {1025, 4},
        {1026, 2},
        {1027, 2},
        {1033, 2},
        {1034, 4},
        {1035, 4},
        {1036, 2},
        {1037, 2},
        {1038, 2},
        {1039, 12},
        {1041, 8},
        {1042, 2},
        {1043, 4},
        {1044, 4},
        {1045, 6},
        {1046, 6},
        {1047, 6},
        {1048, 4},
        {1049, 2},
        {1050, 8},
        {1051, 6},
        {1052, 10},
        {1053, 8},
        {1054, 6},
        {1055, 18},
        {1056, 10},
        {1057, 14},
        {1058, 4},
        {1059, 16},
        {1060, 4},
        {1061, 20},
        {1062, 12},
        {1063, 16},
        {1064, 14},
        {1065, 14},
        {1066, 24},
        {1067, 10},
        {1068, 16},
        {1069, 23},
        {1070, 22},
        {1071, 18},
        {1072, 39},
        {1073, 31},
        {1074, 44},
        {1075, 56},
        {1076, 46},
        {1077, 78},
        {1078, 54},
        {1079, 52},
        {1080, 44},
        {1081, 26},
        {1082, 12},
        {1083, 26},
        {1084, 24},
        {1085, 22},
        {1086, 20},
        {1087, 16},
        {1088, 32},
        {1089, 40},
        {1090, 24},
        {1091, 26},
        {1092, 64},
        {1093, 58},
        {1094, 34},
        {1095, 30},
        {1096, 16},
        {1097, 28},
        {1098, 26},
        {1099, 26},
        {1100, 34},
        {1101, 58},
        {1102, 38},
        {1103, 34},
        {1104, 48},
        {1105, 40},
        {1106, 48},
        {1107, 30},
        {1108, 30},
        {1109, 32},
        {1110, 40},
        {1111, 48},
        {1112, 36},
        {1113, 30},
        {1114, 46},
        {1115, 34},
        {1116, 42},
        {1117, 30},
        {1118, 60},
        {1119, 44},
        {1120, 66},
        {1121, 36},
        {1122, 56},
        {1123, 44},
        {1124, 52},
        {1125, 32},
        {1126, 60},
        {1127, 25},
        {1128, 48},
        {1129, 40},
        {1130, 40},
        {1131, 38},
        {1132, 52},
        {1133, 30},
        {1134, 44},
        {1135, 36},
        {1136, 54},
        {1137, 52},
        {1138, 46},
        {1139, 50},
        {1140, 40},
        {1141, 40},
        {1142, 52},
        {1143, 30},
        {1144, 36},
        {1145, 34},
        {1146, 42},
        {1147, 52},
        {1148, 50},
        {1149, 44},
        {1150, 44},
        {1151, 38},
        {1152, 50},
        {1153, 44},
        {1154, 44},
        {1155, 48},
        {1156, 34},
        {1157, 66},
        {1158, 60},
        {1159, 36},
        {1160, 46},
        {1161, 36},
        {1162, 62},
        {1163, 40},
        {1164, 40},
        {1165, 46},
        {1166, 38},
        {1167, 28},
        {1168, 48},
        {1169, 46},
        {1170, 43},
        {1171, 44},
        {1172, 32},
        {1173, 44},
        {1174, 38},
        {1175, 36},
        {1176, 38},
        {1177, 30},
        {1178, 40},
        {1179, 46},
        {1180, 44},
        {1181, 26},
        {1182, 38},
        {1183, 26},
        {1184, 32},
        {1185, 32},
        {1186, 26},
        {1187, 42},
        {1188, 28},
        {1189, 20},
        {1190, 24},
        {1191, 12},
        {1192, 28},
        {1193, 12},
        {1194, 16},
        {1195, 16},
        {1196, 24},
        {1197, 26},
        {1198, 12},
        {1199, 18},
        {1200, 12},
        {1201, 4},
        {1202, 8},
        {1203, 6},
        {1204, 6},
        {1205, 2},
        {1206, 2},
        {1207, 2},
        {1208, 4},
        {1209, 2},
        {1210, 12},
        {1212, 2},
        {1214, 6},
        {1217, 2},
        {1218, 4},
        {1219, 2},
        {1220, 4},
        {1221, 6},
        {1222, 2},
        {1224, 6},
        {1225, 4},
        {1227, 8},
        {1228, 10},
        {1229, 14},
        {1230, 4},
        {1231, 4},
        {1232, 4},
        {1233, 4},
        {1234, 2},
        {1236, 6},
        {1237, 8},
        {1238, 8},
        {1239, 8},
        {1240, 2},
        {1241, 4},
        {1242, 6},
        {1243, 2},
        {1244, 6},
        {1245, 2},
        {1246, 8},
        {1247, 4},
        {1248, 8},
        {1249, 12},
        {1250, 6},
        {1251, 6},
        {1252, 6},
        {1253, 8},
        {1254, 2},
        {1257, 2},
        {1258, 12},
        {1259, 14},
        {1260, 4},
        {1261, 6},
        {1262, 8},
        {1263, 4},
        {1264, 6},
        {1265, 10},
        {1266, 8},
        {1267, 4},
        {1268, 14},
        {1269, 2},
        {1270, 10},
        {1271, 10},
        {1272, 16},
        {1273, 8},
        {1274, 8},
        {1275, 8},
        {1276, 2},
        {1277, 2},
        {1278, 2},
        {1279, 10},
        {1280, 4},
        {1281, 4},
        {1282, 6},
        {1283, 8},
        {1284, 8},
        {1285, 10},
        {1286, 4},
        {1287, 8},
        {1288, 4},
        {1289, 10},
        {1290, 10},
        {1291, 6},
        {1292, 6},
        {1293, 2},
        {1294, 8},
        {1295, 8},
        {1296, 8},
        {1297, 10},
        {1298, 4},
        {1299, 10},
        {1300, 8},
        {1301, 48},
        {1302, 8},
        {1303, 10},
        {1304, 12},
        {1305, 8},
        {1306, 10},
        {1307, 2},
        {1308, 6},
        {1309, 30},
        {1310, 4},
        {1311, 6},
        {1312, 4},
        {1313, 10},
        {1314, 10},
        {1315, 8},
        {1316, 10},
        {1317, 16},
        {1318, 8},
        {1319, 6},
        {1320, 22},
        {1321, 12},
        {1322, 2},
        {1323, 20},
        {1324, 6},
        {1325, 27},
        {1326, 10},
        {1327, 6},
        {1328, 10},
        {1329, 6},
        {1330, 10},
        {1331, 10},
        {1332, 4},
        {1333, 18},
        {1334, 6},
        {1335, 12},
        {1336, 10},
        {1337, 6},
        {1338, 10},
        {1339, 6},
        {1340, 18},
        {1341, 8},
        {1342, 16},
        {1343, 2},
        {1344, 6},
        {1345, 10},
        {1346, 12},
        {1347, 6},
        {1348, 2},
        {1349, 10},
        {1350, 14},
        {1351, 8},
        {1352, 4},
        {1353, 12},
        {1354, 8},
        {1355, 6},
        {1356, 8},
        {1357, 16},
        {1358, 6},
        {1359, 6},
        {1360, 8},
        {1361, 12},
        {1362, 14},
        {1363, 10},
        {1364, 6},
        {1365, 12},
        {1366, 6},
        {1367, 6},
        {1368, 4},
        {1369, 8},
        {1370, 14},
        {1371, 14},
        {1372, 10},
        {1373, 12},
        {1374, 14},
        {1375, 4},
        {1376, 12},
        {1377, 6},
        {1378, 16},
        {1379, 14},
        {1380, 12},
        {1381, 16},
        {1382, 16},
        {1383, 8},
        {1384, 8},
        {1385, 10},
        {1386, 10},
        {1387, 18},
        {1388, 21},
        {1389, 18},
        {1390, 21},
        {1391, 12},
        {1392, 22},
        {1393, 10},
        {1394, 4},
        {1395, 18},
        {1396, 16},
        {1397, 16},
        {1398, 14},
        {1399, 18},
        {1400, 8},
        {1401, 12},
        {1402, 2},
        {1403, 14},
        {1404, 10},
        {1405, 22},
        {1406, 12},
        {1407, 4},
        {1408, 12},
        {1409, 16},
        {1410, 10},
        {1411, 16},
        {1412, 26},
        {1413, 22},
        {1414, 18},
        {1415, 8},
        {1416, 10},
        {1417, 10},
        {1418, 26},
        {1419, 10},
        {1420, 18},
        {1421, 14},
        {1422, 12},
        {1423, 16},
        {1424, 18},
        {1425, 6},
        {1426, 8},
        {1427, 14},
        {1428, 16},
        {1429, 24},
        {1430, 16},
        {1431, 28},
        {1432, 18},
        {1433, 12},
        {1434, 20},
        {1435, 24},
        {1436, 20},
        {1437, 14},
        {1438, 10},
        {1439, 30},
        {1440, 20},
        {1441, 22},
        {1442, 30},
        {1443, 12},
        {1444, 16},
        {1445, 14},
        {1446, 26},
        {1447, 12},
        {1448, 18},
        {1449, 24},
        {1450, 28},
        {1451, 18},
        {1452, 20},
        {1453, 20},
        {1454, 28},
        {1455, 34},
        {1456, 30},
        {1457, 20},
        {1458, 18},
        {1459, 24},
        {1460, 12},
        {1461, 32},
        {1462, 22},
        {1463, 16},
        {1464, 20},
        {1465, 26},
        {1466, 34},
        {1467, 16},
        {1468, 20},
        {1469, 20},
        {1470, 28},
        {1471, 20},
        {1472, 14},
        {1473, 12},
        {1474, 42},
        {1475, 12},
        {1476, 80},
        {1477, 70},
        {1478, 22},
        {1479, 28},
        {1480, 30},
        {1481, 96},
        {1482, 68},
        {1483, 22},
        {1484, 36},
        {1485, 20},
        {1486, 34},
        {1487, 30},
        {1488, 48},
        {1489, 36},
        {1490, 28},
        {1491, 40},
        {1492, 32},
        {1493, 28},
        {1494, 88},
        {1495, 100},
        {1496, 28},
        {1497, 36},
        {1498, 32},
        {1499, 24},
        {1500, 38},
        {1501, 18},
        {1502, 26},
        {1503, 16},
        {1504, 42},
        {1505, 40},
        {1506, 38},
        {1507, 22},
        {1508, 22},
        {1509, 32},
        {1510, 46},
        {1511, 30},
        {1512, 32},
        {1513, 32},
        {1514, 52},
        {1515, 44},
        {1516, 26},
        {1517, 34},
        {1518, 46},
        {1519, 38},
        {1520, 42},
        {1521, 42},
        {1522, 52},
        {1523, 46},
        {1524, 32},
        {1525, 28},
        {1526, 50},
        {1527, 40},
        {1528, 18},
        {1529, 36},
        {1530, 38},
        {1531, 44},
        {1532, 30},
        {1533, 32},
        {1534, 86},
        {1535, 88},
        {1536, 52},
        {1537, 52},
        {1538, 34},
        {1539, 40},
        {1540, 78},
        {1541, 118},
        {1542, 40},
        {1543, 38},
        {1544, 44},
        {1545, 60},
        {1546, 22},
        {1547, 54},
        {1548, 56},
        {1549, 38},
        {1550, 40},
        {1551, 72},
        {1552, 42},
        {1553, 44},
        {1554, 42},
        {1555, 48},
        {1556, 58},
        {1557, 46},
        {1558, 52},
        {1559, 64},
        {1560, 50},
        {1561, 38},
        {1562, 58},
        {1563, 64},
        {1564, 26},
        {1565, 60},
        {1566, 48},
        {1567, 62},
        {1568, 62},
        {1569, 44},
        {1570, 42},
        {1571, 54},
        {1572, 28},
        {1573, 28},
        {1603, 2},
        {1604, 2},
        {1612, 2},
        {3054, 14},
        {4026, 6},
        {4096, 20},
        {7150, 13},
        {9319, 3},
    };

    const auto max_count{static_cast<double>(raw_histogram.begin()->second)}; // 1 byte is the most common
    const auto SCALING_FACTOR{100'000U};

    FastRandomContext frc{/*fDeterministic=*/true};
    std::vector<std::vector<std::byte>> test_data;
    test_data.reserve(20 * raw_histogram.size());
    size_t total_bytes{0};
    for (const auto& [size, count] : raw_histogram) {
        const size_t scaled_count{static_cast<size_t>(std::ceil((static_cast<double>(count) / max_count) * SCALING_FACTOR))};
        total_bytes += scaled_count * size;
        for (size_t i{0}; i < scaled_count; ++i) {
            test_data.push_back(frc.randbytes<std::byte>(size));
        }
    }
    std::shuffle(test_data.begin(), test_data.end(), frc); // Make it more realistic & less predictable
    assert(total_bytes == 4'317'223);

    auto key{frc.rand64()};
    std::vector<std::byte> key_bytes{8};
    std::memcpy(key_bytes.data(), &key, 8);

    bench.batch(total_bytes).unit("byte").run([&] {
        for (size_t offset = 0; offset < 10; ++offset) {
            for (auto& data : test_data) {
                util::Xor(data, key_bytes, offset);
            }
        }
        ankerl::nanobench::doNotOptimizeAway(test_data);
    });
}

static void AutoFileXor(benchmark::Bench& bench)
{
    FastRandomContext frc{/*fDeterministic=*/true};
    auto data{frc.randbytes<std::byte>(4'000'000)};
    auto key{frc.rand64()};
    std::vector<std::byte> key_bytes(8);
    std::memcpy(&key, key_bytes.data(), 8);

    const fs::path test_path = fs::temp_directory_path() / "xor_benchmark.dat";
    AutoFile f{fsbridge::fopen(test_path, "wb+"), key_bytes};
    bench.batch(data.size()).unit("byte").run([&] {
        f.Truncate(0);
        f << data; // xor through serialization
    });
    try { fs::remove(test_path); } catch (const fs::filesystem_error&) {}
}

BENCHMARK(XorHistogram, benchmark::PriorityLevel::HIGH);
BENCHMARK(AutoFileXor, benchmark::PriorityLevel::LOW);
