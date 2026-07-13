#include "portal_screen.h"

PortalScreen portalScreenForStationCount(int stationCount) {
    return stationCount > 0 ? PortalScreen::UrlGuide : PortalScreen::ApJoin;
}
