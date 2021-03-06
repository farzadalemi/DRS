#include "stdafx.h"
#include "VGroup.h"
#include "DataClasses.h"

using namespace std;

VGroup::VGroup(Trip& t)
{
	leader = &t;
	trips.push_back(&t);
}

VGroup::~VGroup()
{
	for (Trip* t : trips)
	{
		t->group = NULL;
	}
}

bool VGroup::canAddTrip(Trip& t2)
{
	if (!t2.shared || (t2.group && t2.group->trips.size() > 1) || all_people[t2.perid].totalScore < sharingRequirement)	//If trip is already sharing with others
		return false;

	if (Maximize)
	{
		if (trips.size() >= MaxPeople)
		{
			return false;
		}
	}
	else
	{
		if (trips.size() >= MinPeople)
		{
			return false;
		}
	}

	
	int numPassengers = 0;
	for (Trip* t1 : trips)	//Sum numPassengers, or return false if the t2 cannot share with a trip in the group
	{
		if (!strictCompare(*t1, t2))
		{
			return false;
		}
	}

	return true;
	/*
	if (Maximize) //Keep returning true until the car is full
		return(numPassengers <= MaxPeople);
	else //Keep returning true until the car has a certain number of people
		return(numPassengers >= MinPeople && numPassengers <= MaxPeople);
		*/

}
//Try to avoid sharing drivers initially

void VGroup::addTrip(Trip& t2)
{
	//Add t2 to the group's trips, set t2 and the leader as doable, delete t2's old group if needed 
	trips.push_back(&t2);
	leader->setDoable(true);

	t2.setDoable(true);

#pragma omp critical
	if (t2.group)
		delete t2.group;

#pragma omp critical
	t2.group = this;

}



void VGroup::removeTrip(Trip& t1)
{
	if (t1.shared)
	{
		t1.shared = false;
		t1.setDoable(false);
		int size = trips.size();
		unshared++;

		if (size == 2)//If t1 shares with one other
		{
			Trip* t2; //Get t2's id from the group
			if (*trips.begin() == &t1)
				t2 = *trips.rbegin();
			else
				t2 = *trips.begin();

			delete t1.group;
			t1.group = new VGroup(t1);//Form a solo group for t1
			
			t2->group = NULL; //And orphan t2
			t2->setDoable(false);
		} 
		else if (size > 2) //if t1 shares with multiple others
		{
			removeFromTrips(t1);	//Remove t1, and give it a new solo group.
			VGroup* group = t1.group;
			bool wasLeader = group->leader == &t1;
			t1.group = new VGroup(t1);

			//Try to find a new driver for the group, if t1 was the old driver
			bool foundNewDriver = false;
			if (wasLeader)
			{
				for (Trip* t2 : group->trips)
				{
					if (DrivingModes[t2->mode])	//If new driver is found, set it as driver and stop looking
					{
						foundNewDriver = true;
						group->leader = t2;
						break;
					}
				}
				if (!foundNewDriver) //If no new driver was found, disband the group
				{
					for (Trip* t2 : group->trips)
					{
						t2->setDoable(false);
						t2->group = new VGroup(*t2);
					}
					delete group;
				}
			}
		}
	}
}

//Search through the vrgoup, find t, remove it
void VGroup::removeFromTrips(Trip& t)
{
	vector<Trip*>::iterator it = trips.begin();
	while (*it != &t)
		it++;
	it = trips.erase(it);
}