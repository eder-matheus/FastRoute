/* Copyright 2014-2017 Rsyn
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
 
#ifndef RANDOM_PLACEMENT_EXAMPLE_OPTO_H
#define RANDOM_PLACEMENT_EXAMPLE_OPTO_H

#include <iostream>
#include <vector>

#include "x/math/lnalg/lnalg.h"
#include "x/math/lnalg/scrm.h"
#include "rsyn/session/Session.h"
#include "rsyn/phy/PhysicalService.h"
#include "rsyn/model/timing/Timer.h"

namespace ICCAD15 {

class RandomPlacementExample : public Rsyn::Process {
private:
	Rsyn::Session session;
	Rsyn::Timer *timer;
	Rsyn::PhysicalService* physical;	
	Rsyn::Design design;
	Rsyn::Module module;
	Rsyn::PhysicalDesign phDesign;

	void runRandomPlacement();
	
public:
	
	virtual bool run(const Rsyn::Json &params);
	
}; // end class

}
#endif
