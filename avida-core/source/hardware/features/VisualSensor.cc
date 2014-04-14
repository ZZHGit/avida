/*
 *  hardware/features/VisualSensor.cc
 *  avida-core
 *
 *  Created by David on 7/24/13 based on VisualSensor.
 *  Copyright 2013 Michigan State University. All rights reserved.
 *  http://avida.devosoft.org/
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
 *  Authors: David M. Bryson <david@programerror.com>
 *
 */

#include "avida/private/hardware/features/VisualSensor.h"

using namespace Avida::Hardware::Features;



VisualSensor::~VisualSensor()
{
  
}


void VisualSensor::resetOrgSensor()
{
  m_return_rel_facing = false;
  m_has_seen_display = false;
  m_soloBounds.Resize(m_world->GetEnvironment().GetResDefLib().GetSize());
}


const VisualSensor::LookResults VisualSensor::SetLooking(cAvidaContext& ctx, LookSettings& in_defs, int facing, int cell_id, bool use_ft)
{
  int& habitat_used = in_defs.habitat;
  int& distance_sought = in_defs.distance;
  int& search_type = in_defs.search_type;
  int& id_sought = in_defs.id_sought;
  
  const Environment::ResourceManager& resource_lib = m_world->GetEnvironment().GetResDefLib();
  const int lib_size = resource_lib.NumResources();
  const int worldx = m_world->GetConfig().WORLD_X.Get();
  const int worldy = m_world->GetConfig().WORLD_Y.Get();
  bool pred_experiment = (m_world->GetConfig().PRED_PREY_SWITCH.Get() > -1);
  int forage = m_organism->GetForageTarget();
  
  if (m_world->GetConfig().LOOK_DISABLE.Get() < 5 && m_world->GetConfig().LOOK_DISABLE.Get() > 0) {
    int org_type = m_world->GetConfig().LOOK_DISABLE_TYPE.Get();
    bool is_target_type = false;
    if (org_type == 0 && !m_organism->IsPreyFT()) is_target_type = true;
    else if (org_type == 1 && m_organism->IsPreyFT()) is_target_type = true;
    else if (org_type == 2) is_target_type = true;
    
    if (is_target_type) {
      int randsign = ctx.GetRandom().GetUInt(0,2) ? -1 : 1;
      int rand = ctx.GetRandom().GetInt(INT_MAX) * randsign;
      int target_reg = m_world->GetConfig().LOOK_DISABLE.Get();
      if (target_reg == 1) habitat_used = rand;
      else if (target_reg == 2) distance_sought = rand;
      else if (target_reg == 3) search_type = rand;
      else if (target_reg == 4) id_sought = rand;
    }
  }
  
  // first reg gives habitat type sought (aligns with org m_target settings and dynamic res habitat types)
  // if sensing food resource, habitat = 0 (basic res)
  // if sensing topography, habitat = 1 (hills)
  // if sensing objects, habitat = 2 (walls)
  // habitat 4 = unhidden den resource
  // habitat -2 = organisms
  // habitat 5 = simulated predator
  // invalid: habitat 3 (res hidden from distance, caught in inst_lookahead), habitat -1 (unassigned)
  
  // default to look for orgs if invalid habitat & predator
  if (pred_experiment && forage <= -2 && !m_world->GetEnvironment().IsHabitat(habitat_used)) habitat_used = -2;
  // default to look for env res if invalid habitat & forager
  else if (!m_world->GetEnvironment().IsHabitat(habitat_used) && habitat_used != -2) habitat_used = 0;
  
  // second reg gives distance sought--arbitrarily capped at half long axis of world--default to 1 if low invalid number, half-world if high
  int max_dist = 0;
  const int long_axis = (int) (max(worldx, worldy) * 0.5 + 0.5);
  m_world->GetConfig().LOOK_DIST.Get() != -1 ? max_dist = m_world->GetConfig().LOOK_DIST.Get() : max_dist = long_axis;
  if (distance_sought < 0) distance_sought = 1;
  else if (distance_sought > max_dist) distance_sought = max_dist;
  
  // third register gives type of search used for food resources (habitat 0) and org hunting (habitat -2)
  // env res search_types (habitat 0): 0 or 1
  // 0 = look for closest edible res (>=1), closest hill/wall, closest simulated predator, or closest den, 1 = count # edible cells/walls/hills & total food res in cells
  // org hunting search types (habitat -2): -2 -1 0 1 2
  // 0 = closest any org, 1 = closest predator, 2 = count predators, -1 = closest prey, -2 = count prey
  // if looking for env res, default to closest edible
  if (habitat_used != -2 && (search_type < 0 || search_type > 1)) search_type = 0;
  // if looking for orgs in predator environment and is prey, default to closest predator
  else if (pred_experiment && habitat_used == -2 && forage > -2 && (search_type < -2 || search_type > 2)) search_type = 1;
  // if looking for orgs in predator environment and is predator, default to look for prey
  else if (pred_experiment && habitat_used == -2 && forage <= -2 && (search_type < -2 || search_type > 2)) search_type = -1;
  // if looking for orgs in non-predator environment, default to closest org of any type
  else if (!pred_experiment && habitat_used == -2 && (search_type < -2 || search_type > 0)) search_type = 0;
  
  // fourth register gives specific instance of resources sought or specific organisms to look for
  // negative numbers == any of current habitat type
  if (id_sought < -1) id_sought = -1;
  // override if using lookFT
  if ( (id_sought < -1 || id_sought >= lib_size) && use_ft && habitat_used != -2) id_sought = forage;
  // if resource search...
  if (habitat_used != -2) {
    // if invalid res id...
    if (id_sought < -1 || id_sought >= lib_size) {
      if (m_world->GetEnvironment().IsHabitat(habitat_used)) id_sought = -1;      // don't override habitat if it's valid -- else can make it hard for org to see walls and other habitats that would not be expected to match their ft
      else if (forage < 0 || forage >= lib_size) id_sought = -1;                             // e.g. predators looking for res or wacky forage target
      else id_sought = forage;
    }
    if (id_sought != -1) habitat_used = resource_lib.GetResDef(id_sought)->GetHabitat();
  }
  // if looking for org...
  else if (habitat_used == -2) {
    bool done_setting_org = false;
    cOrganism* target_org = NULL;
    // if invalid number or self, we will just search for any org matching search type, skipping rest of look for specific org
    if (id_sought < -1 || id_sought == m_organism->GetID()) {
      id_sought = -1;
      done_setting_org = true;
    }
    // if valid org id number, does the value represent a living organism
    else if (id_sought != -1) {
      const Apto::Array<cOrganism*, Apto::Smart> live_orgs = m_organism->GetOrgInterface().GetLiveOrgList();
      for (int i = 0; i < live_orgs.GetSize(); i++) {
        cOrganism* living_org = live_orgs[i];
        if (id_sought == living_org->GetID()) {
          target_org = living_org;
          done_setting_org = true;
          break;
        }
      }
    }
    // if number didn't represent a living org, we default to walkCells searching for anybody, skipping findOrg
    if (!done_setting_org && id_sought != -1) id_sought = -1;
    // if sought org was is in live org list, we jump to findOrg, skipping walkCells (search_type ignored for this case)
    if (done_setting_org && id_sought != -1) return findOrg(ctx, target_org, distance_sought, facing);
  }
  
  /*  APW TODO
   // add ability to specify minimum distances
   // fifth register modifies search type = look for resource cells with requested food res height value (default = 'off')
   int spec_value = -1;
   const int spec_value_reg = FindModifiedNextRegister(res_id_reg);
   spec_value = m_threads[m_cur_thread].reg[spec_value_reg].value;
   // add ability to target specific forager type
   */
  
  // habitat is 0 and any of the resources are non-dynamic res types, are we dealing with global resources and can just use the global val
  if (habitat_used == 0 || habitat_used > 5) {
    if (id_sought != -1 && resource_lib.GetResDef(id_sought)->GetGeometry() == nGeometry::GLOBAL) {
      return globalVal(ctx, habitat_used, id_sought, search_type);
    }
    else if (id_sought == -1) {
      bool all_global = true;
      for (int i = 0; i < lib_size; i++) {
        if (resource_lib.GetResDef(i)->GetGeometry() == nGeometry::GLOBAL) {
          VisualSensor::LookResults globalval = globalVal(ctx, habitat_used, i, search_type);
          if (globalval.value >= 1 && search_type == 0) return globalval;
        }
        else if (resource_lib.GetResDef(i)->GetGeometry() != nGeometry::GLOBAL && (resource_lib.GetResDef(i)->GetHabitat() == 0 || resource_lib.GetResDef(i)->GetHabitat() > 5)) {
          all_global = false;
          if (search_type == 1) break;
        }
      }
      id_sought = -1;
      if (all_global) return globalVal(ctx, in_defs);       // if all global, but none edible
    }
  }
  return walkCells(ctx, in_defs, facing, cell_id);
}

VisualSensor::LookResults VisualSensor::findOrg(cAvidaContext& ctx, Structure::Element& target_org, const int distance_sought, const int facing)
{
  LookResults org_search;
  org_search.report_type = 1;
  org_search.habitat = -2;
  org_search.id_sought = target_org->GetID();
  org_search.search_type = -9;
  if (m_use_avatar && m_use_avatar != 2) return org_search;
  
  int target_org_cell = target_org->GetOrgInterface().GetCellID();
  if (m_use_avatar) target_org_cell = target_org->GetOrgInterface().GetAVCellID();
  FindThing(target_org_cell, distance_sought, facing, org_search, target_org);
  
  if (org_search.distance > -1) {
    org_search.value = (int) target_org->GetPhenotype().GetCurBonus();
    if (m_return_rel_facing) org_search.group = ReturnRelativeFacing(target_org);
    org_search.forage = target_org->GetForageTarget();
    if ((target_org->IsDisplaying() || m_world->GetConfig().USE_DISPLAY.Get()) && target_org->GetOrgDisplayData() != NULL) SetLastSeenDisplay(target_org->GetOrgDisplayData());
    if (m_world->GetConfig().USE_DISPLAY.Get() == 0 || m_world->GetConfig().USE_DISPLAY.Get() == 1) setPotentialDisplayData(org_search);
    if (m_world->GetConfig().USE_MIMICS.Get() && org_search.forage == 1) {
      if (ctx.GetRandom().P(m_world->GetConfig().MIMIC_ODDS.Get())) org_search.forage = target_org->GetShowForageTarget();
    }
  }
  return org_search;
}

VisualSensor::LookResults VisualSensor::FindResCenter(cAvidaContext& ctx, const int res_id, const int distance_sought, const int facing)
{
  LookResults res_search;
  res_search.report_type = 1;
  res_search.habitat = -2;
  res_search.id_sought = res_id;
  res_search.search_type = -9;
  
  int target_cell = m_organism->GetOrgInterface().GetCellID();
  if (m_use_avatar) target_cell = m_organism->GetOrgInterface().GetAVCellID();
  
  cResourceCount* res_count = m_organism->GetOrgInterface().GetResourceCount();
  if (res_search.distance != -1) {
    if (m_world->GetEnvironment().GetResourceLib().GetResource(res_id)->GetGradient()) {
      target_cell = res_count->GetCurrPeakX(ctx, res_id) + (res_count->GetCurrPeakY(ctx, res_id) * m_world->GetConfig().WORLD_X.Get());
    }
  }
  FindThing(target_cell, distance_sought, facing, res_search);
  return res_search;
}

void VisualSensor::FindThing(int target_cell, const int distance_sought, const int facing, VisualSensor::LookResults& thing_search, cOrganism* target_org)
{
  const int worldx = m_world->GetConfig().WORLD_X.Get();
  const int target_x = target_cell % worldx;
  const int target_y = target_cell / worldx;
  int searching_org_cell = m_organism->GetOrgInterface().GetCellID();
  if (m_use_avatar) {
    searching_org_cell = m_organism->GetOrgInterface().GetAVCellID();
  }
  const int searching_x = searching_org_cell % worldx;
  const int searching_y = searching_org_cell / worldx;
  const int x_dist = target_x - searching_x;
  const int y_dist = target_y - searching_y;
  // is the target close enough to see and in my line of sight?
  bool thing_in_sight = true;
  
  const int travel_dist = max(abs(x_dist), abs(y_dist));
  
  // if simply too far or behind you
  if (travel_dist > distance_sought) thing_in_sight = false;
  else if (facing == 0 && y_dist > 0) thing_in_sight = false;
  else if (facing == 4 && y_dist < 0) thing_in_sight = false;
  else if (facing == 2 && x_dist < 0) thing_in_sight = false;
  else if (facing == 6 && x_dist > 0) thing_in_sight = false;
  else if (facing == 1 && (y_dist > 0 || x_dist < 0)) thing_in_sight = false;
  else if (facing == 3 && (y_dist < 0 || x_dist < 0)) thing_in_sight = false;
  else if (facing == 5 && (y_dist < 0 || x_dist > 0)) thing_in_sight = false;
  else if (facing == 7 && (y_dist > 0 || x_dist > 0)) thing_in_sight = false;
  
  // if not too far in absolute x or y directions, check the distance when we consider offset from center sight line (is it within sight cone?)
  if (thing_in_sight) {
    const int num_cells_either_side = (travel_dist % 2) ? (int) ((travel_dist - 1) * 0.5) : (int) (travel_dist * 0.5);
    int center_cell_x = 0;
    int center_cell_y = 0;
    // facing N or S and target off to E/W of center sight line
    if ((facing == 0 || facing == 4) && abs(x_dist) > num_cells_either_side) thing_in_sight = false;
    // facing E or W and target off to N/S of center sight line
    else if ((facing == 2 || facing == 6) && abs(y_dist) > num_cells_either_side) thing_in_sight = false;
    // if facing diagonals and target off to side
    else if (facing == 1) {
      center_cell_x = searching_x + abs(x_dist);
      center_cell_y = searching_y - abs(y_dist);
      if ((target_x < center_cell_x - num_cells_either_side) || (target_y > center_cell_y + num_cells_either_side)) thing_in_sight = false;
    }
    else if (facing == 3) {
      center_cell_x = searching_x + abs(x_dist);
      center_cell_y = searching_y + abs(y_dist);
      if ((target_x < center_cell_x - num_cells_either_side) || (target_y < center_cell_y - num_cells_either_side)) thing_in_sight = false;
    }
    else if (facing == 5) {
      center_cell_x = searching_x - abs(x_dist);
      center_cell_y = searching_y + abs(y_dist);
      if ((target_x > center_cell_x + num_cells_either_side) || (target_y < center_cell_y - num_cells_either_side)) thing_in_sight = false;
    }
    else if (facing == 7) {
      center_cell_x = searching_x - abs(x_dist);
      center_cell_y = searching_y - abs(y_dist);
      if ((target_x > center_cell_x + num_cells_either_side) || (target_y > center_cell_y + num_cells_either_side)) thing_in_sight = false;
    }
  }
  
  if (thing_in_sight) {
    int offset = x_dist;
    if (facing == 4) offset = -1 * x_dist;
    else if (facing == 2) offset = -1 * y_dist;
    else if (facing == 6) offset = y_dist;
    else if (facing == 1) offset = abs(x_dist) - abs(y_dist);
    else if (facing == 3) offset = abs(y_dist) - abs(x_dist);
    else if (facing == 5) offset = abs(x_dist) - abs(y_dist);
    else if (facing == 7) offset = abs(y_dist) - abs(x_dist);
    thing_search.distance = travel_dist;
    thing_search.deviance = offset;
  }
}

VisualSensor::LookResults VisualSensor::globalVal(cAvidaContext& ctx, LookSettings& in_defs)
{
  double val = 0;
  if (in_defs.id_sought != -1) {
    if (!m_use_avatar) val = m_organism->GetOrgInterface().GetResourceVal(ctx, in_defs.id_sought);
    else if (m_use_avatar) val = m_organism->GetOrgInterface().GetAVResourceVal(ctx, in_defs.id_sought);
  }
  
  LookResults stuff_seen;
  stuff_seen.report_type = 1;
  stuff_seen.habitat = in_defs.habitat;
  stuff_seen.search_type = in_defs.search_type;
  stuff_seen.id_sought = in_defs.id_sought;
  
  // can't use threshold...those only apply to dynamic resources, so this is arbitrarily set at any (> 0)
  if (val > 0) {
    stuff_seen.distance = 0;
    stuff_seen.count = 1;
    stuff_seen.value = (int) (val + 0.5);
    stuff_seen.group = in_defs.id_sought;
    if (m_world->GetConfig().USE_DISPLAY.Get() == 0 || m_world->GetConfig().USE_DISPLAY.Get() == 1) setPotentialDisplayData(stuff_seen);
  }
  return stuff_seen;
}

VisualSensor::LookResults VisualSensor::walkCells(cAvidaContext& ctx, const Environment::ResourceManager& resource_lib, LookSettings& in_defs,
                                           const int facing, const int cell)
{
  // rather than doing doupdates at every cell check inside TestCell, we just do it once now since we're in a stall
  // we need to do this before getfrozenres and getfrozenpeak
  m_organism->GetOrgInterface().TriggerDoUpdates(ctx);
  
  int& habitat_used = in_defs.habitat;
  int& distance_sought = in_defs.distance;
  int& search_type = in_defs.search_type;
  int& id_sought = in_defs.id_sought;
  
  // START definitions
  LookResults stuff_seen;
  stuff_seen.report_type = 0;
  stuff_seen.habitat = habitat_used;
  stuff_seen.search_type = search_type;
  stuff_seen.id_sought = id_sought;
  if ((m_use_avatar && m_use_avatar != 2 && habitat_used == -2) || (habitat_used == 3)) return stuff_seen;
  
  const int worldx = m_world->GetConfig().WORLD_X.Get();
  const int worldy = m_world->GetConfig().WORLD_Y.Get();
  
  int dist_used = distance_sought;
  int start_dist = 0;
  int end_dist = distance_sought;
  
  bool diagonal = true;
  if (facing == 0 || facing == 2 || facing == 4 || facing == 6) diagonal = false;
  
  int faced_cell_int = m_organism->GetOrgInterface().GetFacedCellID();
  if (m_use_avatar) faced_cell_int = m_organism->GetOrgInterface().GetAVFacedCellID();
  
  Apto::Coord<int> faced_cell(faced_cell_int % worldx, faced_cell_int / worldx);
  
  bool do_left = true;
  bool do_right = true;
  bool count_center = true;
  bool any_valid_side_cells = false;
  bool found = false;
  bool found_edible = false;
  int count = 0;
  double totalAmount = 0;
  Apto::Coord<int> first_success_cell(-1, -1);
  int first_whole_resource = -9;
  
  bool stop_at_first_found = (search_type == 0) || (habitat_used == -2 && (search_type == -1 || search_type == 1));
  
  // Key for facings
  // 7 0 1
  // 6 * 2
  // 5 4 3
  Apto::Coord<int> left(0, 0);
  Apto::Coord<int> right(0, 0);
  switch (facing) {
    case 0:
    case 4:
      // Facing North or South
      left.Set(-1, 0);
      right.Set(1, 0);
      break;
      
    case 2:
    case 6:
      // Facing East or West
      left.Set(0, -1);
      right.Set(0, 1);
      break;
      
    case 1:
      //Facing NorthEast
      left.Set(-1, 0);
      right.Set(0, 1);
      break;
    case 3:
      // Facing SouthEast
      left.Set(0, -1);
      right.Set(-1, 0);
      break;
    case 5:
      // Facing SouthWest
      left.Set(1, 0);
      right.Set(0, -1);
      break;
    case 7:
      // Facing NorthWest
      left.Set(0, 1);
      right.Set(1, 0);
      break;
  }
  
  Apto::Coord<int> center_cell(cell % worldx, cell / worldx);
  Apto::Coord<int> this_cell = center_cell;
  Apto::Coord<int> direction = left;
  const Apto::Coord<int> ahead_dir(faced_cell.X() - this_cell.X(), faced_cell.Y() - this_cell.Y());
  
  SearchInfo cellResultInfo;
  int firstVisibleID;
  Apto::Coord<int> firstVisibleCell;
  bool foundFirstVisible = false;
  
  Apto::Array<int, Apto::Smart> val_res;                                                     // resource ids of this habitat type
  // END definitions
  
  bool single_bound = ((habitat_used == 0 || habitat_used >= 4) && id_sought != -1 && m_res_lib.GetResource(id_sought)->GetGradient());
  if (habitat_used != -2 && habitat_used != 3) buildResArray(val_res, in_defs, resource_lib, single_bound);
  
  // set geometric bounds, and fast-forward, if possible
  Bounds worldBounds;
  worldBounds.min_x = 0;
  worldBounds.min_y = 0;
  worldBounds.max_x = worldx - 1;
  worldBounds.max_y = worldy - 1;
  
  Bounds tot_bounds;
  tot_bounds.min_x = worldx - 1;
  tot_bounds.min_y = worldy - 1;
  tot_bounds.max_x = -1 * (worldx - 1);
  tot_bounds.max_y = -1 * (worldy - 1);
  
  m_soloBounds.SetAll(worldBounds);
  
  Apto::Array<int, Apto::Smart> val_res2 = val_res;
  if (habitat_used != -2 && habitat_used != 3) {
    bool has_global = false;
    bool global_only = true;
    int temp_start_dist = distance_sought;
    for (int i = 0; i < val_res.GetSize(); i++) {
      if (resource_lib.GetResDef(val_res[i])->IsDynamic()) {
        int this_start_dist = 0;
        Bounds res_bounds = getBounds(ctx, val_res[i]);
        this_start_dist = getMinDist(worldx, res_bounds, cell, distance_sought, facing);
        // drop any out of range...
        if (this_start_dist == -1) {
          val_res.Swap(i, val_res.GetSize() - 1);
          val_res.Pop();
          i--;
        } else {
          global_only = false;
          if (res_bounds.min_x < tot_bounds.min_x) tot_bounds.min_x = res_bounds.min_x;
          if (res_bounds.min_y < tot_bounds.min_y) tot_bounds.min_y = res_bounds.min_y;
          if (res_bounds.max_x > tot_bounds.max_x) tot_bounds.max_x = res_bounds.max_x;
          if (res_bounds.max_y > tot_bounds.max_y) tot_bounds.max_y = res_bounds.max_y;
          if (this_start_dist < temp_start_dist) temp_start_dist = this_start_dist;
        }
      } else {
        // if any res is global, we just need to make sure we check at least one cell
        if (resource_lib.GetResDef(val_res[i])->GetGeometry() == nGeometry::GLOBAL) has_global = true;
        // if any res is spatial and non-dynamic, we can't bound things because those res don't track the variables we use for bounding
        else {
          global_only = false;
          tot_bounds = worldBounds;
          temp_start_dist = 0;
          break;
        }
      }
    }
    start_dist = temp_start_dist;
    if (val_res.GetSize() == 0) {     // nothing in range
      stuff_seen.report_type = 0;
      return stuff_seen;
    }
    end_dist = getMaxDist(worldx, cell, distance_sought, tot_bounds);
    center_cell += (ahead_dir * start_dist);
    if (has_global && global_only) {
      end_dist = 0;
      start_dist = 0;
      tot_bounds = worldBounds;
    }
  } // END set bounds & fast-forward
  else if (habitat_used == -2) tot_bounds = worldBounds;
  
  // START WALKING
  bool first_step = true;
  for (int dist = start_dist; dist <= end_dist; dist++) {
    if (!testBounds(center_cell, worldBounds) || ((habitat_used != -2 && habitat_used != 3) && !testBounds(center_cell, tot_bounds))) count_center = false;
    // if looking l,r,u,d and center_cell is outside of the world -- we're done with both sides and center
    if (!diagonal && !count_center) break;
    
    // work on SIDE of center cells for this distance
    int num_cells_either_side = 0;
    if (dist > 0) num_cells_either_side = (dist % 2) ? (int) ((dist - 1) * 0.5) : (int) (dist * 0.5);
    // look left then right
    direction = left;
    for (int do_lr = 0; do_lr <= 1; do_lr++) {
      if (do_lr == 1) direction = right;
      if (!do_left && direction == left) continue;
      if (!do_right && direction == right) break;
      
      // walk in from the farthest cell on side towards the center
      for (int j = num_cells_either_side; j > 0; j--) {
        bool valid_cell = true;
        this_cell = center_cell + direction * j;
        if (!testBounds(this_cell, worldBounds) || ((habitat_used != -2 && habitat_used != 3) && !testBounds(center_cell, tot_bounds))) {
          // on diagonals...if any side cell is beyond specific parts of world bounds, we can exclude this side for this and any larger distances
          if (diagonal) {
            const int tcx = this_cell.X();
            const int tcy = this_cell.Y();
            if (direction == left) {
              if ( (facing == 1 && tcy < worldBounds.min_y) || (facing == 3 && tcx > worldBounds.max_x) ||
                  (facing == 5 && tcy > worldBounds.max_y) || (facing == 7 && tcx < worldBounds.min_x) ||
                  (facing == 1 && tcy < tot_bounds.min_y) || (facing == 3 && tcx > tot_bounds.max_x) ||
                  (facing == 5 && tcy > tot_bounds.max_y) || (facing == 7 && tcx < tot_bounds.min_x) ) {
                do_left = false;                         // this cell is out of bounds, and any cells this side of center at any walk dist greater than this will be too
              }
            }
            else if (direction == right) {
              if ( (facing == 1 && tcx > worldBounds.max_x) || (facing == 3 && tcy > worldBounds.max_y) ||
                  (facing == 5 && tcx < worldBounds.min_x) || (facing == 7 && tcy < worldBounds.min_y) ||
                  (facing == 1 && tcx > tot_bounds.max_x) || (facing == 3 && tcy > tot_bounds.max_y) ||
                  (facing == 5 && tcx < tot_bounds.min_x) || (facing == 7 && tcy < tot_bounds.min_y) ) {
                do_right = false;                        // this cell is out of bounds, and any cells this side of center at any walk dist greater than this will be too
              }
            }
            break;                                       // if not !do_left or !do_right, any cells on this side closer than this to center will be too at this distance, but not greater dist
          }
          else if (!diagonal) valid_cell = false;        // when not on diagonal, center cell and cells close(r) to center can still be valid even if this side cell is not
        }
        else any_valid_side_cells = true;
        
        // Now we can look at the current side cell because we know it's in the world.
        if (valid_cell) {
          cellResultInfo = testCell(ctx, in_defs, this_cell, val_res, first_step, stop_at_first_found);
          first_step = false;
          
          if (!foundFirstVisible && cellResultInfo.has_some) {
            firstVisibleID = cellResultInfo.resource_id;
            firstVisibleCell = this_cell;
            foundFirstVisible = true;
          }
          
          if (cellResultInfo.amountFound > 0) {
            found = true;
            totalAmount += cellResultInfo.amountFound;
            if (cellResultInfo.has_edible) {
              count++;                                                         // count cells with individual edible resources (not sum of res in cell >= threshold)
              found_edible = true;
              if (first_success_cell == Apto::Coord<int>(-1, -1)) first_success_cell = this_cell;
              if (first_whole_resource == -9) first_whole_resource = cellResultInfo.resource_id;
              if (stop_at_first_found) {
                dist_used = dist;
                break;                                                          // end search this side
              }
            }
          }
        }
      }
      if (stop_at_first_found && found_edible) break;                           // end both side searches
    }
    if (stop_at_first_found && found_edible) break;                             // end side and center searches (found on side)
    
    // work on CENTER cell for this dist
    if (count_center) {
      cellResultInfo = testCell(ctx, in_defs, center_cell, val_res, first_step, stop_at_first_found);
      
      if (!foundFirstVisible && cellResultInfo.has_some) {
        firstVisibleID = cellResultInfo.resource_id;
        firstVisibleCell = center_cell;
        foundFirstVisible = true;
      }
      
      first_step = false;
      if (cellResultInfo.amountFound > 0) {
        found = true;
        totalAmount += cellResultInfo.amountFound;
        if (cellResultInfo.has_edible) {
          count ++;                                                             // count cells with individual edible resources (not sum of res in cell >=1)
          found_edible = true;
          if (first_success_cell == Apto::Coord<int>(-1, -1)) first_success_cell = center_cell;
          if (first_whole_resource == -9) first_whole_resource = cellResultInfo.resource_id;
          if (stop_at_first_found) {
            dist_used = dist;
            break;                                                              // end side and center searches (found in center)
          }
        }
      }
    }
    // before we check cells at the next distance...
    // stop if we never found any valid cells at the current distance; valid dist_used was previous set of cells checked
    if (!any_valid_side_cells && !count_center) {
      dist--;
      dist_used = dist;
      break;
    }
    center_cell = center_cell + ahead_dir;
  } // END WALKING
  
  // begin reached end output
  stuff_seen.habitat = habitat_used;
  stuff_seen.search_type = search_type;
  stuff_seen.id_sought = id_sought;
  
  if (!found && !foundFirstVisible) {
    stuff_seen.report_type = 0;
  } else if (!found && foundFirstVisible) {
    stuff_seen.report_type = 1;
    stuff_seen.distance = dist_used;
    stuff_seen.count = 0;
    stuff_seen.value = 0;
    stuff_seen.group = firstVisibleID;
    
    int target_cell = firstVisibleCell.Y() * worldx + firstVisibleCell.X();
    FindThing(target_cell, distance_sought, facing, stuff_seen);
    
  } else if (found) {
    stuff_seen.report_type = 1;
    stuff_seen.distance = dist_used;
    stuff_seen.count = count;
    stuff_seen.value = (int) (totalAmount);
    stuff_seen.group = -9;
    stuff_seen.forage = -9;
    
    // overwrite defaults for more specific search types
    
    if (habitat_used != -2) {
      // if we were looking for resources, return id of nearest
      stuff_seen.group = first_whole_resource;
      
      int target_cell = first_success_cell.Y() * worldx + first_success_cell.X();
      FindThing(target_cell, distance_sought, facing, stuff_seen);
      
    } else if (habitat_used == -2 && found_edible) {
      // if searching for orgs, return info on closest one we encountered (==only one if stop_at_first_found)
      const cPopulationCell* first_good_cell = m_organism->GetOrgInterface().GetCell(first_success_cell.Y() * worldx + first_success_cell.X());
      cOrganism* first_org = first_good_cell->GetOrganism();
      if (m_use_avatar) {
        if (search_type == 0) first_org = first_good_cell->GetRandAV(ctx);
        else if (search_type > 0) first_org = first_good_cell->GetRandPredAV();
        else if (search_type < 0) first_org = first_good_cell->GetRandPreyAV();
      }
      
      FindThing(first_good_cell->GetID(), distance_sought, facing, stuff_seen, first_org);
      
      stuff_seen.id_sought = first_org->GetID();
      stuff_seen.value = (int) first_org->GetPhenotype().GetCurBonus();
      if (m_return_rel_facing) stuff_seen.group = ReturnRelativeFacing(first_org);
      stuff_seen.forage = first_org->GetForageTarget();
      if ((first_org->IsDisplaying()  || m_world->GetConfig().USE_DISPLAY.Get()) && first_org->GetOrgDisplayData() != NULL) SetLastSeenDisplay(first_org->GetOrgDisplayData());
      if (m_world->GetConfig().USE_MIMICS.Get() && stuff_seen.forage == 1) {
        if (ctx.GetRandom().P(m_world->GetConfig().MIMIC_ODDS.Get())) stuff_seen.forage = first_org->GetShowForageTarget();
      }
    }
    
    if (m_world->GetConfig().USE_DISPLAY.Get() == 0 || m_world->GetConfig().USE_DISPLAY.Get() == 1) setPotentialDisplayData(stuff_seen);
  }
  return stuff_seen;
}

/* Tests a cell for the Look instructions
 *
 * Returns:
 *		If we're looking for the closest resource, return that resource's ID
 *    otherwise, returns the number of objects we're looking for that are in target_cell
 *
 */
VisualSensor::SearchInfo VisualSensor::testCell(cAvidaContext& ctx, const cResourceDefLib& resource_lib, LookSettings& in_defs,
                                             const Apto::Coord<int>& target_cell_coords,
                                             const Apto::Array <int, Apto::Smart>& val_res, bool first_step,
                                             bool stop_at_first_found)
{
  const int worldx = m_world->GetConfig().WORLD_X.Get();
  int target_cell_num = target_cell_coords.X() + (target_cell_coords.Y() * worldx);
  SearchInfo returnInfo;
  
  int& habitat_used = in_defs.habitat;
  int& search_type = in_defs.search_type;
  
  // if looking for resources or topological features
  if (habitat_used != -2 && habitat_used != 3) {
    // look at every resource ID of this habitat type in the array of resources of interest that we built
    // if counting edible (search_type == 0), return # edible units in each cell, not raw values
    for (int k = 0; k < val_res.GetSize(); k++) {
      if (!testBounds(target_cell_coords, m_soloBounds[val_res[k]])) continue;
      double cell_res = m_organism->GetOrgInterface().GetFrozenCellResVal(ctx, target_cell_num, val_res[k]);
      double edible_threshold = resource_lib.GetResDef(val_res[k])->GetThreshold();
      
      if (cell_res > 0) returnInfo.has_some = true;
      
      if (habitat_used == 0 || habitat_used > 5) {
        if (search_type == 0 && cell_res >= edible_threshold) {
          if (!returnInfo.has_edible) returnInfo.resource_id = val_res[k];                                          // get FIRST whole resource id
          returnInfo.has_edible = true;
          if (first_step || resource_lib.GetResDef(val_res[k])->GetGeometry() != nGeometry::GLOBAL) {             // avoid counting global res more than once (ever)
            returnInfo.amountFound += floor(cell_res / edible_threshold);
          }
        } else if (search_type == 1 && cell_res < edible_threshold && cell_res > 0) {         // only get sum amounts when < threshold if search = get counts
          if (first_step || resource_lib.GetResDef(val_res[k])->GetGeometry() != nGeometry::GLOBAL) {             // avoid counting global res more than once (ever)
            returnInfo.amountFound += cell_res;
          }
        }
      }
      else if ((habitat_used == 1 || habitat_used == 2) && cell_res > 0) {                              // hills and walls work with any vals > 0, not the threshold default of 1
        if (!returnInfo.has_edible) returnInfo.resource_id = val_res[k];
        returnInfo.has_edible = true;
        returnInfo.amountFound += cell_res;
      }
      else if (habitat_used == 5 && cell_res > 0) {                                                   // simulated predators work with any vals > 0 and have chance of detection failing
        if (ctx.GetRandom().P(resource_lib.GetResDef(val_res[k])->GetDetectionProb())) {
          if (!returnInfo.has_edible) returnInfo.resource_id = val_res[k];
          returnInfo.has_edible = true;
          returnInfo.amountFound += cell_res;
        }
      }
      else if (habitat_used == 4) {
        if (search_type == 0 && cell_res >= edible_threshold) {                                       // dens only work above a config set level, but threshold will override this for OrgSensor
          if (!returnInfo.has_edible) returnInfo.resource_id = val_res[k];
          returnInfo.has_edible = true;
          returnInfo.amountFound += floor(cell_res / edible_threshold);
        }
        else if (search_type == 1 && cell_res < edible_threshold && cell_res > 0) {
          returnInfo.amountFound += cell_res;
        }
      }
      if (stop_at_first_found && returnInfo.has_edible) break;
    }
  }
  // if we're looking for other organisms (looking for specific org already handled)
  else if (habitat_used == -2) {
    const cPopulationCell* target_cell = m_organism->GetOrgInterface().GetCell(target_cell_num);
    if (!m_use_avatar) {
      if (target_cell->IsOccupied() && !target_cell->GetOrganism()->IsDead()) {
        int type_seen = target_cell->GetOrganism()->GetForageTarget();
        if (search_type == 0) {
          returnInfo.amountFound++;
          returnInfo.has_edible = true;
        }
        else if (search_type > 0){
          if (type_seen == -2) {
            returnInfo.amountFound++;
            returnInfo.has_edible = true;
          }
        }
        else if (search_type < 0){
          if(type_seen != -2) {
            returnInfo.amountFound++;
            returnInfo.has_edible = true;
          }
        }
      }
    }
    if (m_use_avatar == 2) {
      if(search_type == 0) {
        if (target_cell->HasAV()) {
          returnInfo.amountFound += target_cell->GetNumAV();
          returnInfo.has_edible = true;
        }
      }
      else if (search_type > 0){
        if (target_cell->HasPredAV()) {
          returnInfo.amountFound += target_cell->GetNumPredAV();
          returnInfo.has_edible = true;
        }
      }
      else if (search_type < 0){
        if (target_cell->HasPreyAV()) {
          returnInfo.amountFound += target_cell->GetNumPreyAV();
          returnInfo.has_edible = true;
        }
      }
    }
  }
  return returnInfo;
}

int VisualSensor::getMinDist(const int worldx, Bounds& bounds, const int cell_id, const int distance_sought, const int facing)
{
  const int org_x = cell_id % worldx;
  const int org_y = cell_id / worldx;
  
  if (org_x <= bounds.max_x && org_x >= bounds.min_x && org_y <= bounds.max_y && org_y >= bounds.min_y) return 0; // standing on it
  
  // now for the direction
  int min_x = bounds.min_x;
  int min_y = bounds.min_y;
  if (min_x < 0) min_x = 0;
  if (min_y < 0) min_y = 0;
  
  int max_x = bounds.max_x;
  int max_y = bounds.max_y;
  if (max_x > m_world->GetConfig().WORLD_X.Get() - 1) max_x = m_world->GetConfig().WORLD_X.Get() - 1;
  if (max_y > m_world->GetConfig().WORLD_Y.Get() - 1) max_y = m_world->GetConfig().WORLD_Y.Get() - 1;
  
  // if completely behind you
  if (facing == 0 && min_y > org_y) return -1;
  else if (facing == 4 && max_y < org_y) return -1;
  else if (facing == 2 && max_x < org_x) return -1;
  else if (facing == 6 && min_x > org_x) return -1;
  
  else if (facing == 1 && (min_y > org_y || max_x < org_x)) return -1;
  else if (facing == 3 && (max_y < org_y || max_x < org_x)) return -1;
  else if (facing == 5 && (max_y < org_y || min_x > org_x)) return -1;
  else if (facing == 7 && (min_y > org_y || min_x > org_x)) return -1;
  
  // if not completely behind you, does min travel distance exceed distance used?
  int travel_dist = 0;
  if (facing == 0) travel_dist = org_y - max_y;
  else if (facing == 4) travel_dist = min_y - org_y;
  else if (facing == 2) travel_dist = min_x - org_x;
  else if (facing == 6) travel_dist = org_x - max_x;
  else if (facing == 1) {
    if (org_x > min_x && org_x < max_x) travel_dist = org_y - max_y;
    else if (org_y > min_y && org_y < max_y) travel_dist = min_x - org_x;
    else travel_dist = max(abs(org_x - min_x), abs(org_y - max_y));
  }
  else if (facing == 3) {
    if (org_x > min_x && org_x < max_x) travel_dist = min_y - org_y;
    else if (org_y > min_y && org_y < max_y) travel_dist = min_x - org_x;
    else travel_dist = max(abs(org_x - min_x), abs(org_y - min_y));
  }
  else if (facing == 5) {
    if (org_x > min_x && org_x < max_x) travel_dist = min_y - org_y;
    else if (org_y > min_y && org_y < max_y) travel_dist = org_x - max_x;
    else travel_dist = max(abs(org_x - max_x), abs(org_y - min_y));
  }
  else if (facing == 7) {
    if (org_x > min_x && org_x < max_x) travel_dist = org_y - max_y;
    else if (org_y > min_y && org_y < max_y) travel_dist = org_x - max_x;
    else travel_dist = max(abs(org_x - max_x), abs(org_y - max_y));
  }
  if (travel_dist > distance_sought) return -1;
  if (travel_dist > 0) return travel_dist;
  
  // still going?
  return 0;
  /*  the following is not implemented because it is not quite correct (e.g. for non-geometric shapes
   // check the distance when we consider offset from center sight line (is it within sight cone?)
   // this requires checks using the farthest distance
   int max_sidex = max(abs(org_x - min_x), abs(org_x - max_x));
   int max_sidey = max(abs(org_y - min_y), abs(org_y - max_y));
   max_sidex = (max_sidex % 2) ? (int) ((max_sidex - 1) * 0.5) : (int) (max_sidex * 0.5);
   max_sidey = (max_sidey % 2) ? (int) ((max_sidey - 1) * 0.5) : (int) (max_sidey * 0.5);
   
   if ((facing == 0 || facing == 4) && (min_x > org_x + max_sidex || max_x < org_x - max_sidex)) return -1;
   else if ((facing == 2 || facing == 6) && (min_y > org_y + max_sidey || max_y < org_y - max_sidey)) return -1;
   
   int center_res_x = (int) ((max_x - min_x)  * 0.5);
   int center_res_y = (int) ((max_y - min_y)  * 0.5);
   int center_cell_x = 0;
   int center_cell_y = 0;
   
   // the following only good for diagonals (with slope = 1)
   if (facing == 1) {
   center_cell_x = org_y + center_res_y;
   center_cell_y = org_x - center_res_x;
   max_sidex = org_y + center_res_y;
   max_sidey = org_x - center_res_x;
   if ((max_x < center_cell_x - max_sidex) || (min_y > center_cell_y + max_sidey)) return -1;
   }
   else if (facing == 3) {
   center_cell_x = org_y + center_res_y;
   center_cell_y = org_x + center_res_x;
   max_sidex = org_y + center_res_y;
   max_sidey = org_x + center_res_x;
   if ((max_x < center_cell_x - max_sidex) || (max_y < center_cell_y - max_sidey)) return -1;
   }
   else if (facing == 5) {
   center_cell_x = org_y - center_res_y;
   center_cell_y = org_x + center_res_x;
   max_sidex = org_y - center_res_y;
   max_sidey = org_x + center_res_x;
   if ((min_x > center_cell_x + max_sidex) || (max_y < center_cell_y - max_sidey)) return -1;
   }
   else if (facing == 7) {
   center_cell_x = org_y - center_res_y;
   center_cell_y = org_x - center_res_x;
   max_sidex = org_y - center_res_y;
   max_sidey = org_x - center_res_x;
   if ((min_x > center_cell_x + max_sidex) || (min_y > center_cell_y + max_sidey)) return -1;
   }
   return distance_sought; */
}

int VisualSensor::getMaxDist(const int worldx, const int cell_id, const int distance_sought, Bounds& bounds)
{
  // this will simply return the maximum possible distance to the farthest boundary
  const int org_x = cell_id % worldx;
  const int org_y = cell_id / worldx;
  
  int x1 = org_x - bounds.max_x;
  int x2 = org_x - bounds.min_x;
  int max_x_disp = max(abs(x1), abs(x2));
  
  int y1 = org_y - bounds.max_y;
  int y2 = org_y - bounds.min_y;
  int max_y_disp = max(abs(y1), abs(y2));
  
  int max_dist = max(max_x_disp, max_y_disp);
  
  return min(max_dist, distance_sought);
}

VisualSensor::Bounds VisualSensor::getBounds(cAvidaContext& ctx, const int res_id)
{
  Bounds res_bounds;
  res_bounds.min_x = 0;
  res_bounds.min_y = 0;
  res_bounds.max_x = m_world->GetConfig().WORLD_X.Get() - 1;
  res_bounds.max_y = m_world->GetConfig().WORLD_Y.Get() - 1;
  
  cPopulationResources* res_count = m_organism->GetOrgInterface().GetResourceCount();
  if (res_count != NULL) {
    int min_x = res_count->GetMinUsedX(res_id);
    int min_y = res_count->GetMinUsedY(res_id);
    int max_x = res_count->GetMaxUsedX(res_id);
    int max_y = res_count->GetMaxUsedY(res_id);
    
    if (min_x >= 0 && min_x < m_world->GetConfig().WORLD_X.Get()) res_bounds.min_x = min_x;
    if (min_y >= 0 && min_y < m_world->GetConfig().WORLD_Y.Get()) res_bounds.min_y = min_y;
    if (max_x >= 0 && max_x < m_world->GetConfig().WORLD_X.Get()) res_bounds.max_x = max_x;
    if (max_y >= 0 && max_y < m_world->GetConfig().WORLD_Y.Get()) res_bounds.max_y = max_y;
    
    // no edible drawn in world
    if (min_x == -1 && min_y == -1 && max_x == -1 && max_y == -1) {
      const int this_cell = m_organism->GetCellID();
      res_bounds.min_x = this_cell % m_world->GetConfig().WORLD_X.Get();
      res_bounds.max_x = res_bounds.min_x;
      res_bounds.min_y = this_cell / m_world->GetConfig().WORLD_X.Get();
      res_bounds.max_y = res_bounds.min_y;
    }
  }
  m_soloBounds[res_id] = res_bounds;
  return res_bounds;
}

void VisualSensor::buildResArray(Apto::Array<int, Apto::Smart>& val_res, LookSettings& in_defs, const cResourceDefLib& resource_lib, bool single_bound)
{
  // for hills and walls, we treat them all as generic and don't allow orgs to select individuals instances of that sort of resource
  val_res.Resize(0);
  if (single_bound) val_res.Push(in_defs.id_sought);
  else if (!single_bound) {
    for (int i = 0; i < resource_lib.GetSize(); i++) {
      if (resource_lib.GetResDef(i)->GetHabitat() == habitat_used) val_res.Push(i);
    }
  }
  return val_res;
}

int VisualSensor::ReturnRelativeFacing(cOrganism* sighted_org) {
  const int target_facing = sighted_org->GetOrgInterface().GetFacedDir();
  const int org_facing = m_organism->GetOrgInterface().GetFacedDir();
  if (m_use_avatar != 0) {
    sighted_org->GetOrgInterface().GetAVFacing();
    m_organism->GetOrgInterface().GetAVFacing();
  }
  int match_heading = target_facing - org_facing;             // to match target's heading, rotate this many times in this direction
  if (match_heading > 4) match_heading -= 8;                  // rotate left x times
  else if (match_heading < -4) match_heading += 8;            // rotate right x times
  else if (abs(match_heading) == 4) match_heading = 4;        // rotating 4 and -4 to look same to org
  return match_heading;
}

void VisualSensor::setPotentialDisplayData(LookResults& stuff_seen)
{
  sOrgDisplay* display_data = m_organism->GetPotentialDisplayData();
  if (display_data == NULL) display_data = new sOrgDisplay;
  
  display_data->distance = findDistanceFromHome();
  display_data->direction = findDirFromHome();
  if (stuff_seen.habitat == -2) display_data->thing_id = stuff_seen.id_sought;
  else display_data->thing_id = stuff_seen.group;
  display_data->value = stuff_seen.value;
  
  m_organism->SetPotentialDisplay(display_data);
}

void VisualSensor::SetLastSeenDisplay(OrgDisplay* seen_display)
{
  m_last_seen_display = *seen_display;
  m_has_seen_display = true;
}

int VisualSensor::findDirFromHome()
{
  // Will return direction from birth cell to current cell (aka direction to get here again) if org never used zero-easterly or zero-northerly.
  // Otherwise will return direction to here from the 'marked' spot where those instructions were executed.
  int easterly = m_organism->GetEasterly();
  int northerly = m_organism->GetNortherly();
  int correct_facing = 0;
  if (northerly > 0 && easterly == 0) correct_facing = 4;
  else if (northerly > 0 && easterly < 0) correct_facing = 5;
  else if (northerly == 0 && easterly < 0) correct_facing = 6;
  else if (northerly < 0 && easterly < 0) correct_facing = 7;
  else if (northerly < 0 && easterly == 0) correct_facing = 0;
  else if (northerly < 0 && easterly > 0) correct_facing = 1;
  else if (northerly == 0 && easterly > 0) correct_facing = 2;
  else if (northerly > 0 && easterly > 0) correct_facing = 3;
  return correct_facing;
}

int VisualSensor::findDistanceFromHome()
{
  // Will return min travel distance from birth cell to current cell if org never used zero-easterly or zero-northerly.
  // Otherwise will return distance to here from the 'marked' spot where those instructions were executed.
  return max(abs(m_organism->GetEasterly()), abs(m_organism->GetNortherly()));
}


inline bool VisualSensor::testBounds(const Apto::Coord<int>& cell_id, sBounds& bounds)
{
  const int curr_x = cell_id.X();
  const int curr_y = cell_id.Y();
  if (curr_x < bounds.min_x || curr_y < bounds.min_y || curr_x > bounds.max_x || curr_y > bounds.max_y) return false;
  return true;
}
