/* Copyright 2014-2018 Rsyn
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "FastRoute.h"
#include "fastroute/fastRoute.h"
#include "phy/PhysicalDesign.h"
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <istream>
#define ADJ 0.20

namespace Rsyn {

bool FastRouteProcess::run(const Rsyn::Json &params) {
        std::vector<FastRoute::NET> result;
        std::string outfile = params.value("outfile", "out.guide");
        design = session.getDesign();
        module = design.getTopModule();
        phDesign = session.getPhysicalDesign();

        std::cout << "Initing grid...\n";
        initGrid();
        std::cout << "Initing grid... Done!\n";

        std::cout << "Setting capacities...\n";
        setCapacities();
        std::cout << "Setting capacities... Done!\n";

        std::cout << "Setting orientation...\n";
        setLayerOrientation();
        std::cout << "Setting orientation... Done!\n";

        std::cout << "Setting spacings and widths...\n";
        setSpacingsAndMinWidth();
        std::cout << "Setting spacings and widths... Done!\n";

        std::cout << "Initing nets...\n";
        initNets();
        std::cout << "Initing nets... Done!\n";

        std::cout << "Adjusting grid...\n";
        setGridAdjustments();
        std::cout << "Adjusting grid... Done!\n";

        std::cout << "Computing adjustments...\n";
        computeAdjustments();
        std::cout << "Computing adjustments... Done!\n";

        std::cout << "Running FastRoute...\n";
        fastRoute.run(result);
        std::cout << "Running FastRoute... Done!\n";

        std::cout << "Writing guides...\n";
        writeGuides(result, outfile);
        std::cout << "Writing guides... Done!\n";

        return 0;
}

void FastRouteProcess::initGrid() {
    int nLayers = 0;
        DBU trackSpacing;

        for (Rsyn::PhysicalLayer phLayer : phDesign.allPhysicalLayers()){
                if (phLayer.getType() != Rsyn::ROUTING)
                        continue;

                nLayers++;
                if (phLayer.getRelativeIndex() != selectedMetal -1)
                        continue;

                Rsyn::PhysicalLayerDirection metalDirection = phLayer.getDirection();

                for (Rsyn::PhysicalTracks phTrack : phDesign.allPhysicalTracks(phLayer)){
                        if (phTrack.getDirection() == (PhysicalTrackDirection) metalDirection)
                                continue;
                        trackSpacing = phTrack.getSpace();
                        break;
                }
        }

        DBU tileSize = trackSpacing* pitchesInTile;

        Rsyn::PhysicalDie phDie = phDesign.getPhysicalDie();
        Bounds dieBounds = phDie.getBounds();

        DBU dieX = dieBounds[UPPER][X] - dieBounds[LOWER][X];
        DBU dieY = dieBounds[UPPER][Y] - dieBounds[LOWER][Y];

        int xGrid = std::ceil((float)dieX/tileSize);
        int yGrid = std::ceil((float)dieY/tileSize);

        fastRoute.setLowerLeft(dieBounds[LOWER][X], dieBounds[LOWER][Y]);
        fastRoute.setTileSize(tileSize, tileSize);
        fastRoute.setGridsAndLayers(xGrid, yGrid, nLayers);

        grid.lower_left_x = dieBounds[LOWER][X];
        grid.lower_left_y = dieBounds[LOWER][Y];
        grid.tile_width = tileSize;
        grid.tile_height = tileSize;
        grid.yGrids = yGrid;
        grid.xGrids = xGrid;
}

void FastRouteProcess::setCapacities() {
        for (PhysicalLayer phLayer : phDesign.allPhysicalLayers()) {
                int vCapacity = 0;
                int hCapacity = 0;

                if (phLayer.getType() != Rsyn::ROUTING)
                        continue;

                if (phLayer.getDirection() == Rsyn::HORIZONTAL) {
                        for (PhysicalTracks phTracks : phDesign.allPhysicalTracks(phLayer)) {
                                if (phTracks.getDirection() != (PhysicalTrackDirection)Rsyn::HORIZONTAL) {
                                        hCapacity = std::floor((float)grid.tile_width/phTracks.getSpace());
                                        break;
                                }
                        }

                        fastRoute.addVCapacity(0, phLayer.getRelativeIndex()+1);
                        fastRoute.addHCapacity(hCapacity, phLayer.getRelativeIndex()+1);

                        vCapacities.push_back(0);
                        hCapacities.push_back(hCapacity);
                } else {
                        for (PhysicalTracks phTracks : phDesign.allPhysicalTracks(phLayer)) {
                                if (phTracks.getDirection() != (PhysicalTrackDirection)Rsyn::VERTICAL) {
                                        vCapacity = std::floor((float)grid.tile_width/phTracks.getSpace());
                                        break;
                                }
                        }

                        fastRoute.addVCapacity(vCapacity, phLayer.getRelativeIndex()+1);
                        fastRoute.addHCapacity(0, phLayer.getRelativeIndex()+1);

                        vCapacities.push_back(vCapacity);
                        hCapacities.push_back(0);
                }
        }
}

void FastRouteProcess::setLayerOrientation() {
        for (PhysicalLayer phLayer : phDesign.allPhysicalLayers()) {
                if (phLayer.getType() != Rsyn::ROUTING)
                        continue;

                if (phLayer.getDirection() == Rsyn::HORIZONTAL) {
                        fastRoute.setLayerOrientation(0);
                } else {
                        fastRoute.setLayerOrientation(2);
                }
                break;
        }
}

void FastRouteProcess::setSpacingsAndMinWidth() {
        int minSpacing = 0;
        int minWidth;

        for (PhysicalLayer phLayer : phDesign.allPhysicalLayers()) {
                if (phLayer.getType() != Rsyn::ROUTING)
                        continue;

                for (PhysicalTracks phTracks : phDesign.allPhysicalTracks(phLayer)) {
                        if (phTracks.getDirection() != (PhysicalTrackDirection)phLayer.getDirection())
                                minWidth = phTracks.getSpace();
                        else
                                continue;
                        fastRoute.addMinSpacing(minSpacing, phLayer.getRelativeIndex()+1);
                        fastRoute.addMinWidth(minWidth, phLayer.getRelativeIndex()+1);
                        fastRoute.addViaSpacing(0, phLayer.getRelativeIndex()+1);
                }
        }
}

void FastRouteProcess::initNets() {
        int idx = 0;
        int numNets = 0;

        for (Rsyn::Net net : module.allNets()) {
                numNets++;
        }

        fastRoute.setNumberNets(numNets);

        for (Rsyn::Net net : module.allNets()) {
                std::vector<FastRoute::PIN> pins;
                for (Rsyn::Pin pin: net.allPins()) {
                        DBUxy pinPosition;
                        int pinLayer;
                        if (pin.getInstanceType() == Rsyn::CELL) {
                                Rsyn::PhysicalLibraryPin phLibPin = phDesign.getPhysicalLibraryPin(pin);
                                Rsyn::Cell cell = pin.getInstance().asCell();
                                Rsyn::PhysicalCell phCell = phDesign.getPhysicalCell(cell);
                                const DBUxy cellPos = phCell.getPosition();
                                const Rsyn::PhysicalTransform &transform = phCell.getTransform(true);

                                for (Rsyn::PhysicalPinGeometry pinGeo : phLibPin.allPinGeometries()) {
                                        if (!pinGeo.hasPinLayer())
                                                continue;
                                        for (Rsyn::PhysicalPinLayer phPinLayer : pinGeo.allPinLayers()) {
                                                pinLayer = phPinLayer.getLayer().getRelativeIndex();

                                                std::vector<DBUxy> pinBdsPositions;
                                                DBUxy bdsPosition;
                                                for (Bounds bds : phPinLayer.allBounds()) {
                                                        bds = transform.apply(bds);
                                                        bds.translate(cellPos);
                                                        bdsPosition = bds.computeCenter();
                                                        getPinPosOnGrid(bdsPosition);
                                                        pinBdsPositions.push_back(bdsPosition);
                                                }

                                                int mostVoted = 0;

                                                for (DBUxy pos : pinBdsPositions) {
                                                        int equals = std::count(pinBdsPositions.begin(),
                                                                        pinBdsPositions.end(), pos);
                                                        if (equals > mostVoted) {
                                                                pinPosition = pos;
                                                                mostVoted = equals;
                                                        }
                                                }
                                        }
                                }
                        } else if (pin.getInstanceType() == Rsyn::PORT) {
                                Rsyn::PhysicalLibraryPin phLibPin = phDesign.getPhysicalLibraryPin(pin);
                                Rsyn::Port port = pin.getInstance().asPort();
                                Rsyn::PhysicalPort phPort = phDesign.getPhysicalPort(port);
                                DBUxy portPos = phPort.getPosition();
                                getPinPosOnGrid(portPos);
                                pinPosition = portPos;
                                pinLayer = phPort.getLayer().getRelativeIndex();
                        }
                        FastRoute::PIN grPin;
                        grPin.x = pinPosition.x;
                        grPin.y = pinPosition.y;
                        grPin.layer = pinLayer+1;
                        pins.push_back(grPin);
                }
                FastRoute::PIN grPins[pins.size()];

                char netName[net.getName().size()+1];
                strcpy(netName, net.getName().c_str());
                int count = 0;
                for (FastRoute::PIN pin : pins) {
                        grPins[count] = pin;
                        count++;
                }

                fastRoute.addNet(netName, idx, pins.size(), 1, grPins);
                idx++;
        }

        fastRoute.initEdges();
}

void FastRouteProcess::setGridAdjustments(){
        grid.perfect_regular_x = false;
        grid.perfect_regular_y = false;

        Rsyn::PhysicalDie phDie = phDesign.getPhysicalDie();
        Bounds dieBounds = phDie.getBounds();
        DBUxy upperDieBounds = dieBounds[UPPER];

        int xGrids = grid.xGrids;
        int yGrids = grid.yGrids;
        int xBlocked = upperDieBounds.x % xGrids;
        int yBlocked = upperDieBounds.y % yGrids;
        float percentageBlockedX = xBlocked/grid.tile_width;
        float percentageBlockedY = yBlocked/grid.tile_height;

        if (xBlocked == 0) {
                grid.perfect_regular_x = true;
        }
        if (yBlocked == 0) {
                grid.perfect_regular_y = true;
        }

        for (Rsyn::PhysicalLayer phLayer : phDesign.allPhysicalLayers()){
                if (phLayer.getType() != Rsyn::ROUTING)
                        continue;

                int layerN = phLayer.getRelativeIndex()+1;
                int newVCapacity = std::floor((float)vCapacities[layerN-1]*percentageBlockedX);
                int newHCapacity = std::floor((float)hCapacities[layerN-1]*percentageBlockedY);

                int numAdjustments = 0;
                for (int i=1; i < yGrids; i++)
                        numAdjustments++;
                for (int i=1; i < xGrids; i++)
                        numAdjustments++;
                fastRoute.setNumAdjustments(numAdjustments);

                if (!grid.perfect_regular_x){
                        for (int i=1; i < yGrids; i++){
                                fastRoute.addAdjustment(xGrids-1, i-1, layerN, xGrids-1, i, layerN, newVCapacity);
                        }
                }
                if (!grid.perfect_regular_y){
                        for (int i=1; i < xGrids; i++){
                                fastRoute.addAdjustment(i-1, yGrids-1, layerN, i, yGrids-1, layerN, newHCapacity);
                        }
                }
        }
}

void FastRouteProcess::computeAdjustments() {
        // Temporary adjustment: fixed percentage per layer
        int xGrids = grid.xGrids;
        int yGrids = grid.yGrids;
        int numAdjustments = 0;

        float percentageBlockedX = ADJ;
        float percentageBlockedY = ADJ;

        for (Rsyn::PhysicalLayer phLayer : phDesign.allPhysicalLayers()) {
                if (phLayer.getType() != Rsyn::ROUTING)
                        continue;

                for (int y=0; y < yGrids; y++){
                        for (int x=0; x < xGrids; x++) {
                            numAdjustments++;
                        }
                }
        }

        numAdjustments *= 2;
        fastRoute.setNumAdjustments(numAdjustments);

        for (Rsyn::PhysicalLayer phLayer : phDesign.allPhysicalLayers()) {
                if (phLayer.getType() != Rsyn::ROUTING)
                        continue;

                int layerN = phLayer.getRelativeIndex()+1;
                int newVCapacity = std::floor((float)vCapacities[layerN-1]*(1 - percentageBlockedX));
                int newHCapacity = std::floor((float)hCapacities[layerN-1]*(1 - percentageBlockedY));

                vCapacities[layerN-1] = newVCapacity;
                hCapacities[layerN-1] = newHCapacity;

                if (hCapacities[layerN-1] != 0) {
                        for (int y=1; y < yGrids; y++) {
                                for (int x=1; x < xGrids; x++) {
                                        fastRoute.addAdjustment(x-1, y-1, layerN, x, y-1, layerN, newHCapacity);
                                }
                        }
                }

                if (vCapacities[layerN-1] != 0) {
                        for (int x=1; x < xGrids; x++) {
                            for (int y=1; y < yGrids; y++) {
                                        fastRoute.addAdjustment(x-1, y-1, layerN, x-1, y, layerN, newVCapacity);
                                }
                        }
                }
        }

        fastRoute.initAuxVar();
}

void FastRouteProcess::writeGuides(const std::vector<FastRoute::NET> &globalRoute, std::string filename) {
        std::ofstream guideFile;
        guideFile.open(filename);
        if (!guideFile.is_open()) {
                std::cout << "Error in writeFile!" << std::endl;
                guideFile.close();
                std::exit(0);
        }

        for (FastRoute::NET netRoute : globalRoute) {
                guideFile << netRoute.name << "\n";
                guideFile << "(\n";
                for (FastRoute::ROUTE route : netRoute.route) {
                        if (route.initLayer == route.finalLayer) {
                                Rsyn::PhysicalLayer phLayer =
                                        phDesign.getPhysicalLayerByIndex(Rsyn::ROUTING, route.initLayer-1);

                                Bounds guideBds;
                                guideBds = globalRoutingToBounds(route);
                                guideFile << guideBds.getLower().x << " "
                                          << guideBds.getLower().y << " "
                                          << guideBds.getUpper().x << " "
                                          << guideBds.getUpper().y << " "
                                          << phLayer.getName() << "\n";
                        } else {
                                if (abs(route.finalLayer - route.initLayer) > 1) {
                                        std::cout << "Error: connection between"
                                                "non-adjacent layers";
                                        std::exit(0);
                                } else {
                                        continue;
                                }
                        }

                }
                guideFile << ")\n";
        }
}

void FastRouteProcess::getPinPosOnGrid(DBUxy &pos) {
        DBU x = pos.x;
        DBU y = pos.y;

        // Computing x and y center:
        DBU gCellId_X = floor(x/grid.tile_width);
        DBU gCellId_Y = floor(y/grid.tile_height);

        if (gCellId_X >= grid.xGrids && grid.perfect_regular_x)
            gCellId_X--;

        if (gCellId_Y >= grid.yGrids && grid.perfect_regular_y)
            gCellId_Y--;

        DBU centerX = (gCellId_X) * grid.tile_width + (grid.tile_width/2);
        DBU centerY = (gCellId_Y) * grid.tile_height + (grid.tile_height/2);

        pos = DBUxy(centerX, centerY);
}

Bounds FastRouteProcess::globalRoutingToBounds(const FastRoute::ROUTE &route) {
        DBU llX = route.initX - (grid.tile_width/2);
        DBU llY = route.initY - (grid.tile_height/2);

        DBU urX = route.finalX + (grid.tile_width/2);
        DBU urY = route.finalY + (grid.tile_height/2);

        DBUxy lowerLeft = DBUxy(llX, llY);
        DBUxy upperRight = DBUxy(urX, urY);

        Bounds routeBds = Bounds(lowerLeft, upperRight);
        return routeBds;
}

}
