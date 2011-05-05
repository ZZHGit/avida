/*
 *  cResourceCount.cc
 *  Avida
 *
 *  Called "resource_count.cc" prior to 12/5/05.
 *  Copyright 1999-2011 Michigan State University. All rights reserved.
 *  Copyright 1993-2001 California Institute of Technology.
 *
 *
 *  This file is part of Avida.
 *
 *  Avida is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *
 *  Avida is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License along with Avida.
 *  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "cResourceCount.h"
#include "cResource.h"
#include "cDynamicCount.h"
#include "cGradientCount.h"

#include "nGeometry.h"

#include <cmath>

using namespace std;

const double cResourceCount::UPDATE_STEP(1.0 / 10000.0);
const double cResourceCount::EPSILON (1.0e-15);
const int cResourceCount::PRECALC_DISTANCE(100);


void FlowMatter(cSpatialCountElem &elem1, cSpatialCountElem &elem2, 
                double inxdiffuse, 
                double inydiffuse, double inxgravity, double inygravity,
                int xdist, int ydist, double dist) {

  /* Routine to calculate the amount of flow from one Element to another.
     Amount of flow is a function of:

       1) Amount of material in each cell (will try to equalize)
       2) Distance between each cell
       3) x and y "gravity"

     This method only effect the delta amount of each element.  The State
     method will need to be called at the end of each time step to complete
     the movement of material.
  */

  double  diff, flowamt, xgravity, xdiffuse, ygravity,  ydiffuse;

  if (((elem1.amount == 0.0) && (elem2.amount == 0.0)) && (dist < 0.0)) return;
  diff = (elem1.amount - elem2.amount);
  if (xdist != 0) {

    /* if there is material to be effected by x gravity */

    if (((xdist>0) && (inxgravity>0.0)) || ((xdist<0) && (inxgravity<0.0))) {
      xgravity = elem1.amount * fabs(inxgravity)/3.0;
    } else {
      xgravity = -elem2.amount * fabs(inxgravity)/3.0;
    }
    
    /* Diffusion uses the diffusion constant x half the difference (as the 
       elements attempt to equalize) / the number of possible neighbors (8) */

    xdiffuse = inxdiffuse * diff / 16.0;
  } else {
    xdiffuse = 0.0;
    xgravity = 0.0;
  }  
  if (ydist != 0) {

    /* if there is material to be effected by y gravity */

    if (((ydist>0) && (inygravity>0.0)) || ((ydist<0) && (inygravity<0.0))) {
      ygravity = elem1.amount * fabs(inygravity)/3.0;
    } else {
      ygravity = -elem2.amount * fabs(inygravity)/3.0;
    }
    ydiffuse = inydiffuse * diff / 16.0;
  } else {
    ydiffuse = 0.0;
    ygravity = 0.0;
  }  

  flowamt = ((xdiffuse + ydiffuse + xgravity + ygravity)/
             (fabs(xdist*1.0) + fabs(ydist*1.0)))/dist;
  elem1.delta -= flowamt;
  elem2.delta += flowamt;
}

cResourceCount::cResourceCount(int num_resources)
  : update_time(0.0)
  , spatial_update_time(0.0)
{
  if(num_resources > 0) {
    SetSize(num_resources);
  }

  return;
}

cResourceCount::cResourceCount(const cResourceCount &rc) {
  *this = rc;

  return;
}

const cResourceCount &cResourceCount::operator=(const cResourceCount &rc) {
  SetSize(rc.GetSize());
  resource_name = rc.resource_name;
  resource_initial = rc.resource_initial;  
  resource_count = rc.resource_count;
  decay_rate = rc.decay_rate;
  inflow_rate = rc.inflow_rate;
  decay_precalc = rc.decay_precalc;
  inflow_precalc = rc.inflow_precalc;
  geometry = rc.geometry;
  
  for (int i = 0; i < rc.spatial_resource_count.GetSize(); i++) { 
    *(spatial_resource_count[i]) = *(rc.spatial_resource_count[i]);
  }
  
  curr_grid_res_cnt = rc.curr_grid_res_cnt;
  curr_spatial_res_cnt = rc.curr_spatial_res_cnt;
  update_time = rc.update_time;
  spatial_update_time = rc.spatial_update_time;
  cell_lists = rc.cell_lists;

  return *this;
}

void cResourceCount::SetSize(int num_resources)
{
  resource_name.ResizeClear(num_resources);
  resource_initial.ResizeClear(num_resources);
  resource_count.ResizeClear(num_resources);
  decay_rate.ResizeClear(num_resources);
  inflow_rate.ResizeClear(num_resources);
  if(num_resources > 0) {
    decay_precalc.ResizeClear(num_resources, PRECALC_DISTANCE+1);
    inflow_precalc.ResizeClear(num_resources, PRECALC_DISTANCE+1);
  }
  geometry.ResizeClear(num_resources);
  
  for(int i = 0; i < spatial_resource_count.GetSize(); i++){
    delete spatial_resource_count[i]; 
  }
  
  spatial_resource_count.ResizeClear(num_resources);
  
  for(int i = 0; i < num_resources; i++){
    spatial_resource_count[i] = new cSpatialResCount(); 
  }
    
  curr_grid_res_cnt.ResizeClear(num_resources);
  curr_spatial_res_cnt.ResizeClear(num_resources);
  cell_lists.ResizeClear(num_resources);
  resource_name.SetAll("");
  resource_initial.SetAll(0.0);
  resource_count.SetAll(0.0);
  decay_rate.SetAll(0.0);
  inflow_rate.SetAll(0.0);
  decay_precalc.SetAll(1.0); // This is 1-inflow, so there should be no inflow by default, JEB
  inflow_precalc.SetAll(0.0);
  geometry.SetAll(nGeometry::GLOBAL);
  curr_grid_res_cnt.SetAll(0.0);
  //DO spacial resources need to be set to zero?
}

cResourceCount::~cResourceCount()
{
  for(int i = 0; i < spatial_resource_count.GetSize(); i++){
    delete spatial_resource_count[i]; 
  }
}

void cResourceCount::SetCellResources(int cell_id, const tArray<double> & res)
{
  assert(resource_count.GetSize() == res.GetSize());

  for (int i = 0; i < resource_count.GetSize(); i++) {
     if (geometry[i] == nGeometry::GLOBAL || geometry[i]==nGeometry::PARTIAL) {
        // Set global quantity of resource
    } else {
      spatial_resource_count[i]->SetCellAmount(cell_id, res[i]);

      /* Ideally the state of the cell's resource should not be set till
         the end of the update so that all processes (inflow, outflow, 
         diffision, gravity and organism demand) have the same weight.  However
         waiting can cause problems with negative resources so we allow
         the organism demand to work immediately on the state of the resource */ 
    }
  }
}

void cResourceCount::Setup(cWorld* world, const int& res_index, const cString& name, const double& initial, const double& inflow, const double& decay,                  
				const int& in_geometry, const double& in_xdiffuse, const double& in_xgravity, 
				const double& in_ydiffuse, const double& in_ygravity,
				const int& in_inflowX1, const int& in_inflowX2, const int& in_inflowY1, const int& in_inflowY2,
				const int& in_outflowX1, const int& in_outflowX2, const int& in_outflowY1, 
				const int& in_outflowY2, tArray<cCellResource> *in_cell_list_ptr,
				tArray<int> *in_cell_id_list_ptr, const int& verbosity_level,
				const bool& isdynamic, const int& in_peaks,
				const double& in_min_height, const double& in_min_radius, const double& in_radius_range,
				const double& in_ah, const double& in_ar,
				const double& in_acx, const double& in_acy,
				const double& in_hstepscale, const double& in_rstepscale,
				const double& in_cstepscalex, const double& in_cstepscaley,
				const double& in_hstep, const double& in_rstep,
				const double& in_cstepx, const double& in_cstepy,
				const int& in_update_dynamic, const int& in_peakx, const int& in_peaky,
				const int& in_height, const int& in_spread, const double& in_plateau, const int& in_decay, 
        const int& in_max_x, const int& in_min_x, const int& in_max_y, const int& in_min_y, const double& in_move_a_scaler,
        const int& in_updatestep, const int& in_halo, const int& in_halo_inner_radius, const int& in_halo_width,
        const int& in_halo_anchor_x, const int& in_halo_anchor_y, const int& in_move_speed,
        const double& in_plateau_inflow, const double& in_plateau_outflow, const int& in_is_plateau_common, 
        const double& in_floor, const bool& isgradient
				)
{
  assert(res_index >= 0 && res_index < resource_count.GetSize());
  assert(initial >= 0.0);
  assert(decay >= 0.0);
  assert(inflow >= 0.0);
  assert(spatial_resource_count[res_index]->GetSize() > 0);
  int tempx = spatial_resource_count[res_index]->GetX();
  int tempy = spatial_resource_count[res_index]->GetY();

  cString geo_name;
  if (in_geometry == nGeometry::GLOBAL) {
    geo_name = "GLOBAL";
  } else if (in_geometry == nGeometry::GRID) {
    geo_name = "GRID";
  } else if (in_geometry == nGeometry::TORUS) {
    geo_name = "TORUS";
  } 
	else if (in_geometry == nGeometry::PARTIAL) {
	geo_name = "PARTIAL";
  }
  else {
    cerr << "[cResourceCount::Setup] Unknown resource geometry " << in_geometry << ".  Exiting.";
    exit(2);
  }


  /* If the verbose flag is set print out information about resources */
  verbosity = verbosity_level;
  if (verbosity > VERBOSE_NORMAL) {
    cout << "Setting up resource " << name
         << "(" << geo_name 
         << ") with initial quatity=" << initial
         << ", decay=" << decay
         << ", inflow=" << inflow
         << endl;
    if ((in_geometry == nGeometry::GRID) || (in_geometry == nGeometry::TORUS)) {
      cout << "  Inflow rectangle (" << in_inflowX1 
           << "," << in_inflowY1 
           << ") to (" << in_inflowX2 
           << "," << in_inflowY2 
           << ")" << endl; 
      cout << "  Outflow rectangle (" << in_outflowX1 
           << "," << in_outflowY1 
           << ") to (" << in_outflowX2 
           << "," << in_outflowY2 
           << ")" << endl;
      cout << "  xdiffuse=" << in_xdiffuse
           << ", xgravity=" << in_xgravity
           << ", ydiffuse=" << in_ydiffuse
           << ", ygravity=" << in_ygravity
           << endl;
    }   
  }
  

  /* recource_count gets only the values for global resources */

  resource_name[res_index] = name;
  resource_initial[res_index] = initial;
  if (in_geometry == nGeometry::GLOBAL) {
    resource_count[res_index] = initial;
	spatial_resource_count[res_index]->RateAll(0);
  } 
  else if (in_geometry == nGeometry::PARTIAL) {
	  resource_count[res_index]=initial;

	  spatial_resource_count[res_index]->RateAll(0);
	  // want to set list of cell ids here
	   cell_lists[res_index].Resize(in_cell_id_list_ptr->GetSize());
	  for (int i = 0; i < in_cell_id_list_ptr->GetSize(); i++)
		  cell_lists[res_index][i] = (*in_cell_id_list_ptr)[i];

  }
  else {
    resource_count[res_index] = 0; 
    if(isdynamic){ //JW
      delete spatial_resource_count[res_index];
      spatial_resource_count[res_index] = new cDynamicCount(in_peaks, in_min_height, in_radius_range, in_min_radius, in_ah, in_ar,
			    in_acx, in_acy, in_hstepscale, in_rstepscale, in_cstepscalex, in_cstepscaley, in_hstep, in_rstep,
			    in_cstepx, in_cstepy, tempx, tempy, in_geometry, in_updatestep); 
      spatial_resource_count[res_index]->RateAll(0);
    }
    
    else if(isgradient){
      delete spatial_resource_count[res_index];
      spatial_resource_count[res_index] = new cGradientCount(world, in_peakx, in_peaky, in_height, in_spread, in_plateau, in_decay,                                
                                                      in_max_x, in_max_y, in_min_x, in_min_y, in_move_a_scaler, in_updatestep, 
                                                      tempx, tempy, in_geometry, in_halo, in_halo_inner_radius, 
                                                      in_halo_width, in_halo_anchor_x, in_halo_anchor_y, in_move_speed,
                                                      in_plateau_inflow, in_plateau_outflow, in_is_plateau_common, in_floor);
      spatial_resource_count[res_index]->RateAll(0);
    }
    
    else{
      spatial_resource_count[res_index]->SetInitial(initial / spatial_resource_count[res_index]->GetSize());
      spatial_resource_count[res_index]->RateAll(spatial_resource_count[res_index]->GetInitial());
    }
  }
  spatial_resource_count[res_index]->StateAll();  
  decay_rate[res_index] = decay;
  inflow_rate[res_index] = inflow;
  geometry[res_index] = in_geometry;
  spatial_resource_count[res_index]->SetGeometry(in_geometry);
  spatial_resource_count[res_index]->SetPointers();
  spatial_resource_count[res_index]->SetCellList(in_cell_list_ptr);

  double step_decay = pow(decay, UPDATE_STEP);
  double step_inflow = inflow * UPDATE_STEP;
  
  decay_precalc(res_index, 0) = 1.0;
  inflow_precalc(res_index, 0) = 0.0;
  for (int i = 1; i <= PRECALC_DISTANCE; i++) {
    decay_precalc(res_index, i)  = decay_precalc(res_index, i-1) * step_decay;
    inflow_precalc(res_index, i) = inflow_precalc(res_index, i-1) * step_decay + step_inflow;
  }
  spatial_resource_count[res_index]->SetXdiffuse(in_xdiffuse);
  spatial_resource_count[res_index]->SetXgravity(in_xgravity);
  spatial_resource_count[res_index]->SetYdiffuse(in_ydiffuse);
  spatial_resource_count[res_index]->SetYgravity(in_ygravity);
  spatial_resource_count[res_index]->SetInflowX1(in_inflowX1);
  spatial_resource_count[res_index]->SetInflowX2(in_inflowX2);
  spatial_resource_count[res_index]->SetInflowY1(in_inflowY1);
  spatial_resource_count[res_index]->SetInflowY2(in_inflowY2);
  spatial_resource_count[res_index]->SetOutflowX1(in_outflowX1);
  spatial_resource_count[res_index]->SetOutflowX2(in_outflowX2);
  spatial_resource_count[res_index]->SetOutflowY1(in_outflowY1);
  spatial_resource_count[res_index]->SetOutflowY2(in_outflowY2);
}

/*
 * This is unnecessary now that a resource has an index
 * TODO: 
 *  - Change name to GetResourceCountIndex
 *  - Fix anything that breaks by just using the index of the resource (not id)
 *  - Get rid of this function
 */
int cResourceCount::GetResourceCountID(const cString& res_name)
{
    for (int i = 0; i < resource_name.GetSize(); i++) {
      if (resource_name[i] == res_name) return i;
    }
    cerr << "Error: Unknown resource '" << res_name << "'." << endl;
    return -1;
}

double cResourceCount::GetInflow(const cString& name)
{
  int id = GetResourceCountID(name);
  if (id == -1) return -1;
  
  return inflow_rate[id];
}

void cResourceCount::SetInflow(const cString& name, const double _inflow)
{
  int id = GetResourceCountID(name);
  if (id == -1) return;

  inflow_rate[id] = _inflow;
  double step_inflow = _inflow * UPDATE_STEP;
  double step_decay = pow(decay_rate[id], UPDATE_STEP);

  inflow_precalc(id, 0) = 0.0;
  for (int i = 1; i <= PRECALC_DISTANCE; i++) {
    inflow_precalc(id, i) = inflow_precalc(id, i-1) * step_decay + step_inflow;
  }
}

double cResourceCount::GetDecay(const cString& name)
{
  int id = GetResourceCountID(name);
  if (id == -1) return -1;
  
  return decay_rate[id];
}

void cResourceCount::SetDecay(const cString& name, const double _decay)
{
  int id = GetResourceCountID(name);
  if (id == -1) return;

  decay_rate[id] = _decay;
  double step_decay = pow(_decay, UPDATE_STEP);
  decay_precalc(id, 0) = 1.0;
  for (int i = 1; i <= PRECALC_DISTANCE; i++) {
    decay_precalc(id, i)  = decay_precalc(id, i-1) * step_decay;
  }
}

void cResourceCount::Update(double in_time) 
{ 
  update_time += in_time;
  spatial_update_time += in_time;
 }

 
const tArray<double> & cResourceCount::GetResources(cAvidaContext& ctx) const 
{
  DoUpdates(ctx); 
  return resource_count;
}
 
const tArray<double> & cResourceCount::GetCellResources(int cell_id, cAvidaContext& ctx) const 

  // Get amount of the resource for a given cell in the grid.  If it is a
  // global resource pass out the entire content of that resource.

{
  int num_resources = resource_count.GetSize();
  DoUpdates(ctx);

  for (int i = 0; i < num_resources; i++) {
     if (geometry[i] == nGeometry::GLOBAL || geometry[i]==nGeometry::PARTIAL) {
         curr_grid_res_cnt[i] = resource_count[i];
    } else {
      curr_grid_res_cnt[i] = spatial_resource_count[i]->GetAmount(cell_id);
    }
  }
  return curr_grid_res_cnt;

}

const tArray<int> & cResourceCount::GetResourcesGeometry() const
{
  return geometry;
}

const tArray< tArray<double> > &  cResourceCount::GetSpatialRes(cAvidaContext& ctx)
{
  const int num_spatial_resources = spatial_resource_count.GetSize();
  if (num_spatial_resources > 0) {
    const int num_cells = spatial_resource_count[0]->GetSize();
    DoUpdates(ctx);
    for (int i = 0; i < num_spatial_resources; i++) {
      for (int j = 0; j < num_cells; j++) {
	curr_spatial_res_cnt[i][j] = spatial_resource_count[i]->GetAmount(j);
      }
    }
  }

  return curr_spatial_res_cnt;
}

void cResourceCount::Modify(const tArray<double> & res_change)
{
  assert(resource_count.GetSize() == res_change.GetSize());

  for (int i = 0; i < resource_count.GetSize(); i++) {
    resource_count[i] += res_change[i];
    assert(resource_count[i] >= 0.0);
  }
}


void cResourceCount::Modify(int res_index, double change)
{
  assert(res_index < resource_count.GetSize());

  resource_count[res_index] += change;
  assert(resource_count[res_index] >= 0.0);
}

void cResourceCount::ModifyCell(const tArray<double> & res_change, int cell_id)
{
  assert(resource_count.GetSize() == res_change.GetSize());

  for (int i = 0; i < resource_count.GetSize(); i++) {
  if (geometry[i] == nGeometry::GLOBAL || geometry[i]==nGeometry::PARTIAL) {
        resource_count[i] += res_change[i];
      assert(resource_count[i] >= 0.0);
    } else {
      double temp = spatial_resource_count[i]->Element(cell_id).GetAmount();
      spatial_resource_count[i]->Rate(cell_id, res_change[i]);

      /* Ideally the state of the cell's resource should not be set till
         the end of the update so that all processes (inflow, outflow, 
         diffision, gravity and organism demand) have the same weight.  However
         waiting can cause problems with negative resources so we allow
         the organism demand to work immediately on the state of the resource */ 
    
      spatial_resource_count[i]->State(cell_id);
      // cout << "BDB in cResourceCount::ModifyCell id = " << i << " cell = " << cell_id << " amount[41] = " << spatial_resource_count[i]->GetAmount(41) << endl;
    
      if(spatial_resource_count[i]->Element(cell_id).GetAmount() != temp){
        spatial_resource_count[i]->SetModified(true);
      }
    }
  }
}

double cResourceCount::Get(int res_index) const
{
cout << res_index << endl;
  assert(res_index < resource_count.GetSize());
  if (geometry[res_index] == nGeometry::GLOBAL || geometry[res_index]==nGeometry::PARTIAL) {
      return resource_count[res_index];
  } //else return spacial resource sum
  return spatial_resource_count[res_index]->SumAll();
}

void cResourceCount::Set(int res_index, double new_level)
{
  assert(res_index < resource_count.GetSize());
  if (geometry[res_index] == nGeometry::GLOBAL || geometry[res_index]==nGeometry::PARTIAL) {
     resource_count[res_index] = new_level;
  } else {
    for(int i = 0; i < spatial_resource_count[res_index]->GetSize(); i++) {
      spatial_resource_count[res_index]->SetCellAmount(i, new_level/spatial_resource_count[res_index]->GetSize());
    }
  }
}

void cResourceCount::ResizeSpatialGrids(int in_x, int in_y)
{
  for (int i = 0; i < resource_count.GetSize(); i++) {
    spatial_resource_count[i]->ResizeClear(in_x, in_y, geometry[i]);
    curr_spatial_res_cnt[i].Resize(in_x * in_y);
  }
}
///// Private Methods /////////
void cResourceCount::DoUpdates(cAvidaContext& ctx) const
{ 
  assert(update_time >= -EPSILON);

  // Determine how many update steps have progressed
  int num_steps = (int) (update_time / UPDATE_STEP);

  // Preserve remainder of update_time
  update_time -=  num_steps * UPDATE_STEP;

  while (num_steps > PRECALC_DISTANCE) {
    for (int i = 0; i < resource_count.GetSize(); i++) {
      if (geometry[i] == nGeometry::GLOBAL || geometry[i]==nGeometry::PARTIAL) {
        resource_count[i] *= decay_precalc(i, PRECALC_DISTANCE);
        resource_count[i] += inflow_precalc(i, PRECALC_DISTANCE);
      }
    }
    num_steps -= PRECALC_DISTANCE;
  }

  for (int i = 0; i < resource_count.GetSize(); i++) {
    if (geometry[i] == nGeometry::GLOBAL || geometry[i]==nGeometry::PARTIAL) {
      resource_count[i] *= decay_precalc(i, num_steps);
      resource_count[i] += inflow_precalc(i, num_steps);
    }
  }

  // If one (or more) complete update has occured update the spatial resources

  while (spatial_update_time >= 1.0) { 
    spatial_update_time -= 1.0;
    for (int i = 0; i < resource_count.GetSize(); i++) {
     if (geometry[i] != nGeometry::GLOBAL && geometry[i] != nGeometry::PARTIAL) {
        spatial_resource_count[i]->UpdateCount(ctx);
        spatial_resource_count[i]->Source(inflow_rate[i]);
        spatial_resource_count[i]->Sink(decay_rate[i]);
        if (spatial_resource_count[i]->GetCellListSize() > 0) {
          spatial_resource_count[i]->CellInflow();
          spatial_resource_count[i]->CellOutflow();
        }
        spatial_resource_count[i]->FlowAll();
        spatial_resource_count[i]->StateAll();
        // BDB: resource_count[i] = spatial_resource_count[i]->SumAll();
      }
    }
  }
}

void cResourceCount::ReinitializeResources(double additional_resource)
{
  for(int i = 0; i < resource_name.GetSize(); i++) {
    Set(i, resource_initial[i] + additional_resource); //will cause problem if more than one resource is used. -- why?  each resource is stored separately (BDC)

    // Additionally, set any initial values given by the CELL command
    spatial_resource_count[i]->ResetResourceCounts();
    if (additional_resource != 0.0) {
      spatial_resource_count[i]->RateAll(additional_resource);
      spatial_resource_count[i]->StateAll();
    }

  } //End going through the resources
}

/* 
 * TODO: This is a duplicate of GetResourceCountID()
 * Follow the same steps to get rid of it.
 */
int cResourceCount::GetResourceByName(cString name) const
{
  int result = -1;
  
  for(int i = 0; i < resource_name.GetSize(); i++)
  {
    if(resource_name[i] == name)
    {
      result = i;
    }
  }
  
  return result;
  
}
