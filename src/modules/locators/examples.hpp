#ifndef EXAMPLES_HPP
#define EXAMPLES_HPP
#include <string>
#include <vector>
#include <urts/services/scalable/locators/uLocator/locationRequest.hpp>
#include <urts/services/scalable/locators/uLocator/arrival.hpp>

namespace
{

[[maybe_unused]]
URTS::Services::Scalable::Locators::ULocator::Arrival
toArrival(const std::string &network,
          const std::string &station,
          const std::string &phase,
          const double time,
          const int64_t identifier)
{
    URTS::Services::Scalable::Locators::ULocator::Arrival arrival;
    arrival.setNetwork(network);
    arrival.setStation(station);
    if (phase == "P")
    {
        arrival.setPhase(URTS::Services::Scalable::Locators::ULocator::Arrival::Phase::P);
        arrival.setStandardError(0.05);
    }
    else
    {
        arrival.setPhase(URTS::Services::Scalable::Locators::ULocator::Arrival::Phase::S);    
        arrival.setStandardError(0.1);
    }
    arrival.setTime(time);
    arrival.setIdentifier(identifier);
    return arrival;
}

[[maybe_unused]]
URTS::Services::Scalable::Locators::ULocator::LocationRequest testUtahEventRequest()
{
    URTS::Services::Scalable::Locators::ULocator::LocationRequest request;
/*
80085261
Catalog:     1729851504.23     39.00063  -111.4063, -1200
Associators: 1729851504.634941 39.027048 -111.401232 1146.080597
[2024-10-25 04:22:38.485] [stdout] [info] 832 UU.WPUT.HHZ.01.P 1729851505.246174
[2024-10-25 04:22:38.485] [stdout] [info] 833 UU.CVRU.HHZ.01.P 1729851508.773843
[2024-10-25 04:22:38.485] [stdout] [info] 834 UU.OWUT.EHZ.01.P 1729851509.607865
[2024-10-25 04:22:38.485] [stdout] [info] 835 UU.SGU.HHZ.01.P 1729851509.895722
[2024-10-25 04:22:38.485] [stdout] [info] 836 UU.TMU.HHZ.01.P 1729851511.046742
[2024-10-25 04:22:38.485] [stdout] [info] 837 UU.CVRU.HHN.01.S 1729851512.289533
[2024-10-25 04:22:38.485] [stdout] [info] 838 UU.SRU.HHZ.01.P 1729851517.556067
[2024-10-25 04:22:38.485] [stdout] [info] 839 UU.LIUT.HHZ.01.P 1729851522.321738
[2024-10-25 04:22:38.485] [stdout] [info] 840 UU.BCE.EHZ.01.P 1729851522.847059
[2024-10-25 04:22:38.485] [stdout] [info] 841 UU.ROA.EHZ.01.P 1729851523.973603
[2024-10-25 04:22:38.485] [stdout] [info] 842 UU.BRPU.HHN.01.S 1729851526.899263
[2024-10-25 04:22:38.485] [stdout] [info] 843 UU.SRU.HHN.01.S 1729851527.116703
*/
    std::vector<URTS::Services::Scalable::Locators::ULocator::Arrival> arrivals{
        ::toArrival("UU", "WPUT", "P", 1729851505.246174, 1),
        ::toArrival("UU", "CVRU", "P", 1729851508.773843, 2),
        ::toArrival("UU", "OWUT", "P", 1729851509.607865, 3),
        ::toArrival("UU", "SGU",  "P", 1729851509.895722, 4),
        ::toArrival("UU", "TMU",  "P", 1729851511.046742, 5),
        ::toArrival("UU", "CVRU", "S", 1729851512.289533, 6),
        ::toArrival("UU", "SRU",  "P", 1729851517.556067, 7),
        ::toArrival("UU", "LIUT", "P", 1729851522.321738, 8),
        ::toArrival("UU", "BCE",  "P", 1729851522.847059, 9),
        ::toArrival("UU", "ROA",  "P", 1729851523.973603, 10),
        ::toArrival("UU", "BRPU", "S", 1729851526.899263, 11),
        ::toArrival("UU", "SRU",  "S", 1729851527.116703, 12)
    };
    request.setIdentifier(1);
    request.setArrivals(arrivals);
    request.setLocationStrategy(URTS::Services::Scalable::Locators::ULocator::LocationRequest::LocationStrategy::General);
    return request;
}

[[maybe_unused]]
URTS::Services::Scalable::Locators::ULocator::LocationRequest testUtahQuarryEventRequest()
{
    URTS::Services::Scalable::Locators::ULocator::LocationRequest request;
/*
80085281
Catalog:  1729878429.40     40.1758   -112.2308  -2000
Origin:   1729878428.647415 40.109618 -112.220657 2368.932496
[2024-10-25 11:51:25.826] [stdout] [info] 917 UU.NLU.HHZ.01.P 1729878435.045524
[2024-10-25 11:51:25.826] [stdout] [info] 918 UU.CWU.EHZ.01.P 1729878435.816268
[2024-10-25 11:51:25.826] [stdout] [info] 919 UU.MID.ENZ.01.P 1729878436.627585
[2024-10-25 11:51:25.826] [stdout] [info] 920 UU.LSU.ENZ.01.P 1729878438.602306
[2024-10-25 11:51:25.826] [stdout] [info] 921 UU.NLU.HHN.01.S 1729878438.718194
[2024-10-25 11:51:25.826] [stdout] [info] 922 UU.COY.ENZ.01.P 1729878439.164146
[2024-10-25 11:51:25.826] [stdout] [info] 923 UU.NOQ.HHZ.01.P 1729878439.424180
[2024-10-25 11:51:25.826] [stdout] [info] 924 UU.SCC.ENZ.01.P 1729878439.517962
[2024-10-25 11:51:25.826] [stdout] [info] 925 UU.MPU.HHZ.01.P 1729878439.886615
[2024-10-25 11:51:25.826] [stdout] [info] 926 UU.GMU.EHZ.01.P 1729878440.353893
[2024-10-25 11:51:25.826] [stdout] [info] 927 UU.CTU.HHZ.01.P 1729878442.137043
[2024-10-25 11:51:25.826] [stdout] [info] 928 UU.LEVU.EHZ.02.P 1729878444.658032
[2024-10-25 11:51:25.826] [stdout] [info] 929 UU.SNUT.EHZ.02.P 1729878444.680917
[2024-10-25 11:51:25.826] [stdout] [info] 930 UU.FLU.EHZ.02.P 1729878444.961002
[2024-10-25 11:51:25.826] [stdout] [info] 931 UU.SUU.EHN.01.S 1729878445.270607
[2024-10-25 11:51:25.826] [stdout] [info] 932 UU.GMO.ENN.01.S 1729878446.168982
[2024-10-25 11:51:25.826] [stdout] [info] 933 UU.HTU.ENZ.01.P 1729878447.022182
[2024-10-25 11:51:25.826] [stdout] [info] 934 UU.RRCU.EHZ.01.P 1729878448.511859
[2024-10-25 11:51:25.826] [stdout] [info] 935 UU.WES.ENN.01.S 1729878450.313926
[2024-10-25 11:51:25.826] [stdout] [info] 936 UU.BSUT.HHZ.01.P 1729878452.819076
[2024-10-25 11:51:25.826] [stdout] [info] 937 UU.LHUT.EHZ.01.P 1729878453.269839
[2024-10-25 11:51:25.826] [stdout] [info] 938 UU.HLJ.HHN.01.S 1729878454.590867
[2024-10-25 11:51:25.826] [stdout] [info] 939 UU.HDUT.HHZ.01.P 1729878458.897102
[2024-10-25 11:51:25.826] [stdout] [info] 940 UU.TCU.HHN.01.S 1729878468.584589
[2024-10-25 11:51:25.826] [stdout] [info] 941 UU.LRG.ENN.01.S 1729878477.716151
*/
    std::vector<URTS::Services::Scalable::Locators::ULocator::Arrival> arrivals{
        ::toArrival("UU", "NLU",  "P", 1729878435.045524, 1),
        ::toArrival("UU", "CWU",  "P", 1729878435.816268, 2),
        ::toArrival("UU", "MID",  "P", 1729878436.627585, 3),
        ::toArrival("UU", "LSU",  "P", 1729878438.602306, 4),
        ::toArrival("UU", "NLU",  "S", 1729878438.718194, 5),
        ::toArrival("UU", "COY",  "P", 1729878439.164146, 6),
        ::toArrival("UU", "NOQ",  "P", 1729878439.424180, 7),
        ::toArrival("UU", "SCC",  "P", 1729878439.517962, 8),
        ::toArrival("UU", "MPU",  "P", 1729878439.886615, 9),
        ::toArrival("UU", "GMU",  "P", 1729878440.353893, 10),
        ::toArrival("UU", "CTU",  "P", 1729878442.137043, 11),
        ::toArrival("UU", "LEVU", "P", 1729878444.658032, 12),
        ::toArrival("UU", "SNUT", "P", 1729878444.680917, 13),
        ::toArrival("UU", "FLU",  "P", 1729878444.961002, 14),
        ::toArrival("UU", "SUU",  "S", 1729878445.270607, 15),
        ::toArrival("UU", "GMO",  "S", 1729878446.168982, 16),
        ::toArrival("UU", "HTU",  "P", 1729878447.022182, 17),
        ::toArrival("UU", "RRCU", "P", 1729878448.511859, 18),
        ::toArrival("UU", "WES",  "S", 1729878450.313926, 19),
        ::toArrival("UU", "BSUT", "P", 1729878452.819076, 20),
        ::toArrival("UU", "LHUT", "P", 1729878453.269839, 21),
        ::toArrival("UU", "HLJ",  "S", 1729878454.590867, 22),
        ::toArrival("UU", "HDUT", "P", 1729878458.897102, 23),
        ::toArrival("UU", "TCU",  "S", 1729878468.584589, 24),
        ::toArrival("UU", "LRG",  "S", 1729878477.716151, 25)
    };
    request.setIdentifier(2);
    request.setArrivals(arrivals);
    request.setLocationStrategy(URTS::Services::Scalable::Locators::ULocator::LocationRequest::LocationStrategy::FreeSurface);
    return request;
}

[[maybe_unused]]
URTS::Services::Scalable::Locators::ULocator::LocationRequest testYNPEventRequest()
{
    URTS::Services::Scalable::Locators::ULocator::LocationRequest request;
/*
80085236
Catalog: 44.7062 -110.8322 5600
[2024-10-24 21:48:59.527] [stdout] [info] Origin 82 1729827886.344657 44.709113 -110.829470 5066.935365
[2024-10-24 21:48:59.527] [stdout] [info] 732 WY.YPM.EHZ.01.P 1729827889.095722
[2024-10-24 21:48:59.527] [stdout] [info] 733 WY.YHH.HHZ.01.P 1729827889.487471
[2024-10-24 21:48:59.527] [stdout] [info] 734 WY.YNB.HHZ.01.P 1729827889.715872
[2024-10-24 21:48:59.527] [stdout] [info] 735 WY.YMR.HHZ.01.P 1729827890.033611
[2024-10-24 21:48:59.527] [stdout] [info] 736 WY.YNR.HHZ.01.P 1729827890.096452
[2024-10-24 21:48:59.527] [stdout] [info] 737 WY.YMC.EHZ.01.P 1729827890.621145
[2024-10-24 21:48:59.527] [stdout] [info] 738 WY.YHH.HHN.01.S 1729827891.022236
[2024-10-24 21:48:59.527] [stdout] [info] 739 WY.YNM.HHN.01.S 1729827891.108295
[2024-10-24 21:48:59.527] [stdout] [info] 740 WY.YNB.HHN.01.S 1729827891.233577
[2024-10-24 21:48:59.527] [stdout] [info] 741 WY.YML.EHZ.01.P 1729827891.473963
[2024-10-24 21:48:59.527] [stdout] [info] 742 WY.YMR.HHN.01.S 1729827891.821105
[2024-10-24 21:48:59.527] [stdout] [info] 743 WY.YNR.HHN.01.S 1729827892.100778
[2024-10-24 21:48:59.527] [stdout] [info] 744 WY.YGC.HHZ.01.P 1729827892.118977
[2024-10-24 21:48:59.527] [stdout] [info] 745 WY.YWB.HHZ.01.P 1729827892.436260
[2024-10-24 21:48:59.527] [stdout] [info] 746 WY.YUF.HHZ.01.P 1729827892.535284
[2024-10-24 21:48:59.527] [stdout] [info] 747 WY.YMC.EHN.01.S 1729827892.841679
[2024-10-24 21:48:59.527] [stdout] [info] 748 WY.YFT.HHZ.01.P 1729827892.847355
[2024-10-24 21:48:59.527] [stdout] [info] 749 WY.YHB.HHZ.01.P 1729827892.946088
[2024-10-24 21:48:59.527] [stdout] [info] 750 WY.YHL.HHZ.01.P 1729827893.423553
[2024-10-24 21:48:59.527] [stdout] [info] 751 WY.YDC.HHZ.01.P 1729827893.495229
[2024-10-24 21:48:59.527] [stdout] [info] 752 WY.YMV.HHZ.01.P 1729827893.531886
[2024-10-24 21:48:59.527] [stdout] [info] 753 WY.YML.EHN.01.S 1729827894.005285
[2024-10-24 21:48:59.527] [stdout] [info] 754 WY.YJC.HHZ.01.P 1729827894.895759
[2024-10-24 21:48:59.527] [stdout] [info] 755 WY.YDD.HHZ.01.P 1729827894.988019
[2024-10-24 21:48:59.527] [stdout] [info] 756 WY.YUF.HHN.01.S 1729827895.976318
[2024-10-24 21:48:59.527] [stdout] [info] 757 WY.YFT.ENN.01.S 1729827896.686913
[2024-10-24 21:48:59.527] [stdout] [info] 758 WY.YHB.HHN.01.S 1729827896.969280
[2024-10-24 21:48:59.527] [stdout] [info] 759 WY.YDC.HHN.01.S 1729827897.748685
[2024-10-24 21:48:59.527] [stdout] [info] 760 WY.YMV.HHN.01.S 1729827897.873758
[2024-10-24 21:48:59.527] [stdout] [info] 761 WY.MCID.EHZ.01.P 1729827899.012296
[2024-10-24 21:48:59.527] [stdout] [info] 762 WY.YDD.HHN.01.S 1729827899.590054
[2024-10-24 21:48:59.527] [stdout] [info] 763 WY.YJC.HHN.01.S 1729827900.126931
[2024-10-24 21:48:59.527] [stdout] [info] 764 WY.YPP.HHN.01.S 1729827903.267945
*/
    std::vector<URTS::Services::Scalable::Locators::ULocator::Arrival> arrivals{
        ::toArrival("WY", "YPM", "P", 1729827889.095722, 1),
        ::toArrival("WY", "YHH", "P", 1729827889.487471, 2),
        ::toArrival("WY", "YNB", "P", 1729827889.715872, 3),
        ::toArrival("WY", "YMR", "P", 1729827890.033611, 4),
        ::toArrival("WY", "YNR", "P", 1729827890.096452, 5),
        ::toArrival("WY", "YMC", "P", 1729827890.621145, 6),
        ::toArrival("WY", "YHH", "S", 1729827891.022236, 7),
        ::toArrival("WY", "YNM", "S", 1729827891.108295, 8),
        ::toArrival("WY", "YNB", "S", 1729827891.233577, 9),
        ::toArrival("WY", "YML", "P", 1729827891.473963, 10),
        ::toArrival("WY", "YMR", "S", 1729827891.821105, 11),
        ::toArrival("WY", "YNR", "S", 1729827892.100778, 12),
        ::toArrival("WY", "YGC", "P", 1729827892.118977, 13),
        ::toArrival("WY", "YWB", "P", 1729827892.436260, 14),
        ::toArrival("WY", "YUF", "P", 1729827892.535284, 15),
        ::toArrival("WY", "YMC", "S", 1729827892.841679, 16),
        ::toArrival("WY", "YFT", "P", 1729827892.847355, 17),
        ::toArrival("WY", "YHB", "P", 1729827892.946088, 18),
        ::toArrival("WY", "YHL", "P", 1729827893.423553, 19),
        ::toArrival("WY", "YDC", "P", 1729827893.495229, 20),
        ::toArrival("WY", "YMV", "P", 1729827893.531886, 21),
        ::toArrival("WY", "YML", "S", 1729827894.005285, 22),
        ::toArrival("WY", "YJC", "P", 1729827894.895759, 23),
        ::toArrival("WY", "YDD", "P", 1729827894.988019, 24),
        ::toArrival("WY", "YUF", "S", 1729827895.976318, 25),
        ::toArrival("WY", "YFT", "S", 1729827896.686913, 26),
        ::toArrival("WY", "YHB", "S", 1729827896.969280, 27),
        ::toArrival("WY", "YDC", "S", 1729827897.748685, 28),
        ::toArrival("WY", "YMV", "S", 1729827897.873758, 29),
        ::toArrival("WY", "MCID","P", 1729827899.012296, 30),
        ::toArrival("WY", "YDD", "S", 1729827899.590054, 31),
        ::toArrival("WY", "YJC", "S", 1729827900.126931, 32),
        ::toArrival("WY", "YPP", "S", 1729827903.267945, 33)
    };
    request.setIdentifier(3);
    request.setArrivals(arrivals);
    request.setLocationStrategy(URTS::Services::Scalable::Locators::ULocator::LocationRequest::LocationStrategy::FreeSurface);
    return request;

}

}
#endif
