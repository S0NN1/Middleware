#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>
#include <math.h>


//Costants from the project assignment
int minTimeToGetInfected = 10 * 60; //10 minutes
int minTimeToHeal = 60 * 60 * 24 * 10; //10 days
int TimeToBecomeSuspceptibleAgain = 60 * 60 * 24 * 90; // 3 months

struct point {
	int x;
	int y;
};

//structure that handles a list and its size
struct arrayWithSize {
	void* pList;
	int currentSize;
	int maxSize;
};

struct individual {
	int rank;
	struct point position;
	int isInfected;
	int lastTimeHeWasInfected;
	int lastTimeHeRecovered;
	int id;
	int subnation;
};

struct contactHistory {
	int timeStart;
	int timeEnd;
	struct individual* from;
	struct individual* to;
};

struct subnation {
	struct arrayWithSize people; //list of individual
	int nRectangles;
	struct point position;
	int rank;
};

struct nation {
	struct arrayWithSize list; //list of subnation
	int rank;
};

struct individualSummary {
	int sane;
	int infected;
};

struct individualSummaryWithRank {
	int rank;
	struct individualSummary individualSummary;
};

struct arrayWithSizeAndIndividual {
	struct arrayWithSize a;
	struct individual* i;
};

void free2(void* p) {
	if (p != NULL)
	{
		free(p);
	}
}

void free3(struct arrayWithSize a) {
	free2(a.pList);
}

void free4(struct nation n) {
	free3(n.list);
}

//method that calculate how many subnations the world must be splitted into: the area must be feasible to have a perfect split
int calculateNumSubnations(int W, int L, int w, int l)
{
	float f = (float)W * L;
	f /= (w * l);

	int i = W * L;
	i /= (w * l);

	if (f == i) {
		int n = (W * L) / (w * l);
		double d1 = (double)n;
		double d2 = sqrt(d1);
		return (int)d2;
	}

	return -1;
}

int getRandomNumber(int lower, int upper) {
	int num = (rand() % (upper - lower + 1)) + lower;
	return num;
}


//method that assign, for each mpi process, a number of subnations each process will handle
struct arrayWithSize calculateDistributionSubnationsToProcess(int numProcess, int numSubnations) {

	int* r = malloc(sizeof(int) * numProcess);

	int i = 0;
	int done = 0;

	for (int j = 0; j < numProcess; j++)
	{
		r[j] = 0;
	}

	while (done < numSubnations) {
		r[i]++;

		i++;

		if (i >= numProcess)
		{
			i = 0;
		}
		done++;
	}

	struct arrayWithSize arrayWithSizeVar;
	arrayWithSizeVar.pList = r;
	arrayWithSizeVar.currentSize = numProcess;
	arrayWithSizeVar.maxSize = numProcess;
	return arrayWithSizeVar;
}

int getMax(struct arrayWithSize a) {
	int max = -1;
	int* p2 = a.pList;
	for (int i = 0; i < a.currentSize; i++)
	{
		if (p2[i] > max)
		{
			max = p2[i];
		}
	}

	return max;
}

//method that populates a subnation with people, calcutaing how many infected, sane people each nation will get
struct individualSummaryWithRank* fillPeopleInformation(int numPeople, int numInfected,
	int numProcess, struct arrayWithSize maxRectanglesForEachProcess, int maxSubnationPerProcess) {
	int sani = numPeople - numInfected;
	if (maxSubnationPerProcess < 0)
		return NULL;

	struct individualSummaryWithRank* returnValue =
		malloc(sizeof(struct individualSummaryWithRank) * numProcess * maxSubnationPerProcess);

	for (int i = 0; i < numProcess; i++) {
		for (int j = 0; j < maxSubnationPerProcess; j++)
		{
			int k = (i * maxSubnationPerProcess) + j;
			returnValue[k].rank = (i * maxSubnationPerProcess) + j;
			returnValue[k].individualSummary.infected = 0;
			returnValue[k].individualSummary.sane = 0;
		}
	}

	int* m2 = maxRectanglesForEachProcess.pList;
	for (int i = 0; i < numInfected; i++)
	{
		int where1 = -1;
		do {
			where1 = getRandomNumber(0, numProcess - 1);
		} while (m2[where1] - 1 < 0);

		int where2 = getRandomNumber(0, m2[where1] - 1);
		int k = (where1 * maxSubnationPerProcess) + where2;
		returnValue[k].individualSummary.infected++;
	}

	for (int i = 0; i < sani; i++)
	{
		int where1 = -1;
		do {
			where1 = getRandomNumber(0, numProcess - 1);
		} while (m2[where1] - 1 < 0);
		int where2 = getRandomNumber(0, m2[where1] - 1);
		int k = (where1 * maxSubnationPerProcess) + where2;
		returnValue[k].individualSummary.sane++;
	}

	return returnValue;
}


//method that inserts and individual to a list
struct arrayWithSize insertIndividual(struct arrayWithSize arrayWithSize, struct individual* individual) {
	while (1)
	{
		struct individual** r = arrayWithSize.pList;

		if (arrayWithSize.currentSize < arrayWithSize.maxSize)
		{
			r[arrayWithSize.currentSize] = individual;
			arrayWithSize.currentSize++;
			return arrayWithSize;
		}

		int newSize = arrayWithSize.maxSize * 2;
		if (newSize < 1)
			newSize = 1;

		struct individual** r2 = malloc(sizeof(struct individual*) * newSize);
		for (int i = 0; i < arrayWithSize.currentSize; i++)
		{
			r2[i] = r[i];
		}

		if (arrayWithSize.pList != NULL)
			free2(arrayWithSize.pList);

		arrayWithSize.pList = r2;
		arrayWithSize.maxSize = newSize;
	}
}


//method that inserts a int to a list
struct arrayWithSize insertInt(struct arrayWithSize arrayWithSize, int value) {
	while (1)
	{
		int* r = arrayWithSize.pList;

		if (arrayWithSize.currentSize < arrayWithSize.maxSize)
		{
			r[arrayWithSize.currentSize] = value;
			arrayWithSize.currentSize++;
			return arrayWithSize;
		}

		int newSize = arrayWithSize.maxSize * 2;
		if (newSize < 1)
			newSize = 1;

		int* r2 = malloc(sizeof(value) * newSize);
		for (int i = 0; i < arrayWithSize.currentSize; i++)
		{
			r2[i] = r[i];
		}

		if (arrayWithSize.pList != NULL)
			free2(arrayWithSize.pList);

		arrayWithSize.pList = r2;
		arrayWithSize.maxSize = newSize;
	}
}


//method that inserts an individual in a list 
struct arrayWithSize insertHashIndividual(
	struct arrayWithSize a,
	struct individual* p)
{
	while (1)
	{
		struct arrayWithSizeAndIndividual* r = a.pList;

		if (a.currentSize < a.maxSize)
		{
			struct arrayWithSizeAndIndividual* rf = malloc(sizeof(struct arrayWithSizeAndIndividual));
			rf->i = p;
			rf->a.currentSize = 0;
			rf->a.maxSize = 0;
			rf->a.pList = NULL;

			r[a.currentSize] = *rf;
			a.currentSize++;
			return a;
		}

		int newSize = a.maxSize * 2;
		if (newSize < 1)
			newSize = 1;

		struct arrayWithSizeAndIndividual* r2 = malloc(sizeof(struct arrayWithSizeAndIndividual) * newSize);
		for (int i = 0; i < a.currentSize; i++)
		{
			r2[i] = r[i];
		}

		if (a.pList != NULL)
			free2(a.pList);

		a.pList = r2;
		a.maxSize = newSize;
	}
}

//method that inserts a contacthistory in a list
struct arrayWithSizeAndIndividual* insertContactHistory(struct arrayWithSizeAndIndividual* arrayWithSize, struct contactHistory* vic) {
	while (1)
	{
		struct contactHistory** r = arrayWithSize->a.pList;

		if (arrayWithSize->a.currentSize < arrayWithSize->a.maxSize)
		{
			r[arrayWithSize->a.currentSize] = vic;
			arrayWithSize->a.currentSize++;
			return arrayWithSize;
		}

		int newSize = arrayWithSize->a.maxSize * 2;
		if (newSize < 1)
			newSize = 1;

		struct contactHistory** r2 = malloc(sizeof(vic) * newSize);
		for (int i = 0; i < arrayWithSize->a.currentSize; i++)
		{
			r2[i] = r[i];
		}

		if (arrayWithSize->a.pList != NULL)
			free2(arrayWithSize->a.pList);

		arrayWithSize->a.pList = r2;
		arrayWithSize->a.maxSize = newSize;
	}
}


//method that inserts a subnation in a list
struct arrayWithSize insertSubnation(struct arrayWithSize arrayWithSize, struct subnation subnation) {
	while (1)
	{
		struct subnation* r = arrayWithSize.pList;

		if (arrayWithSize.currentSize < arrayWithSize.maxSize)
		{
			r[arrayWithSize.currentSize] = subnation;
			arrayWithSize.currentSize++;
			return arrayWithSize;
		}

		int newSize = arrayWithSize.maxSize * 2;
		if (newSize < 1)
			newSize = 1;

		struct subnation* r2 = malloc(sizeof(subnation) * newSize);
		for (int i = 0; i < arrayWithSize.currentSize; i++)
		{
			r2[i] = r[i];
		}

		if (arrayWithSize.pList != NULL)
			free2(arrayWithSize.pList);

		arrayWithSize.pList = r2;
		arrayWithSize.maxSize = newSize;
	}
}


//method that generates the nation map of a mpi process. Each subnation has a position, a rank and a number of infected and sane individuals.
struct nation GenerateMap(
	int rank,
	struct arrayWithSize howManySubNationsPerProcess,
	int w, int l,
	struct individualSummaryWithRank* start,
	int maxRectanglesPerProcess)
{
	struct nation n;
	n.rank = rank;
	struct arrayWithSize arrayWithSizeVar;
	arrayWithSizeVar.currentSize = 0;
	arrayWithSizeVar.maxSize = 0;
	arrayWithSizeVar.pList = NULL;
	n.list = arrayWithSizeVar;

	int* p1 = howManySubNationsPerProcess.pList;
	for (int i = 0; i < p1[rank]; i++)
	{
		struct subnation r;
		r.position.x = i % w;
		r.position.y = i / l;
		r.rank = rank;
		r.nRectangles = w * l;

		r.people.currentSize = 0;
		r.people.maxSize = 0;
		r.people.pList = NULL;

		int k2 = i;

		for (int k = 0; k < start[k2].individualSummary.infected; k++) {
			struct individual* personItem = malloc(sizeof(struct individual));
			personItem->rank = rank;
			personItem->isInfected = 1;
			personItem->position.x = getRandomNumber(0, w);
			personItem->position.y = getRandomNumber(0, l);
			personItem->lastTimeHeWasInfected = 0;
			personItem->lastTimeHeRecovered = -1;
			personItem->id = r.people.currentSize;
			personItem->subnation = k2;
			r.people = insertIndividual(r.people, personItem);
		}

		for (int k = 0; k < start[k2].individualSummary.sane; k++)
		{
			struct individual* personItem = malloc(sizeof(struct individual));
			personItem->rank = rank;
			personItem->isInfected = 0;
			personItem->position.x = getRandomNumber(0, w);
			personItem->position.y = getRandomNumber(0, l);
			personItem->lastTimeHeWasInfected = -1;
			personItem->lastTimeHeRecovered = -1;
			personItem->id = r.people.currentSize;
			personItem->subnation = k2;
			r.people = insertIndividual(r.people, personItem);
		}

		n.list = insertSubnation(n.list, r);
	}

	return n;
}


//method that gets all the people of a nation
struct arrayWithSize getPeople(struct nation nationItem) {
	struct arrayWithSize people;
	people.currentSize = 0;
	people.maxSize = 0;
	people.pList = NULL;

	struct subnation* p2 = nationItem.list.pList;
	for (int i = 0; i < nationItem.list.currentSize; i++)
	{
		struct individual** p3 = p2[i].people.pList;
		for (int j = 0; j < p2[i].people.currentSize; j++)
		{
			people = insertIndividual(people, p3[j]);
		}
	}

	return people;
}


//method that prints infected/sane information to the console
void printInfectedInformation(struct individualSummaryWithRank i2, int maxRectanglesPerProcess, int i) {
	if (maxRectanglesPerProcess == 1)
	{
		printf("  >TOT: INFECTED %d, SANE %d\n", i2.individualSummary.infected, i2.individualSummary.sane);
	}
	else {
		int index = (i2.rank - ((i / maxRectanglesPerProcess) * maxRectanglesPerProcess));
		if (index >= 10)
			printf("  > %d: INFECTED %d, SANE %d\n", index, i2.individualSummary.infected, i2.individualSummary.sane);
		else
			printf("  >  %d: INFECTED %d, SANE %d\n", index, i2.individualSummary.infected, i2.individualSummary.sane);
	}
}

//method that prints infected/sane information to the console
void printArray(struct arrayWithSize buffer3, int maxRectanglesPerProcess) {
	struct individualSummaryWithRank* p2 = buffer3.pList;
	int totInfectedPartial = 0;
	int totSanePartial = 0;
	int totInfectedTotal = 0;
	int totSaneTotal = 0;
	for (int i = 0; i < buffer3.currentSize; i++)
	{
		struct individualSummaryWithRank p3 = p2[i];
		if (i % maxRectanglesPerProcess == 0)
		{
			totInfectedPartial = 0;
			totSanePartial = 0;
			printf(" Rank: %d\n", i / maxRectanglesPerProcess);
		}

		(totInfectedPartial) += p3.individualSummary.infected;
		(totSanePartial) += p3.individualSummary.sane;
		totInfectedTotal += p3.individualSummary.infected;
		totSaneTotal += p3.individualSummary.sane;

		printInfectedInformation(p3, maxRectanglesPerProcess, i);

		if (maxRectanglesPerProcess > 0)
		{
			if ((i + 1) % maxRectanglesPerProcess == 0)
			{
				printf("  >TOT: INFECTED %d, SANE %d\n", (totInfectedPartial), (totSanePartial));
			}
		}
	}

	printf(">TOT: INFECTED %d, SANE %d\n", (totInfectedTotal), (totSaneTotal));
}

//method that gets all the subnation of a nation, given the rank
struct arrayWithSize GetSubnations(struct nation nationItem, int rank) {
	struct arrayWithSize r;
	r.currentSize = 0;
	r.maxSize = 0;
	r.pList = NULL;

	struct subnation* p2 = nationItem.list.pList;
	for (int i = 0; i < nationItem.list.currentSize; i++)
	{
		if (p2[i].rank == rank)
		{
			r = insertSubnation(r, p2[i]);
		}
	}

	return r;
}

//method that, given the coordinates, return the index of the rectangle in a grid (counting cells)
int rectIndex(int x, int y, int w, int l) {
	return (x * w) + y;
}

//method that returns all the cell indexes of the cell near a cell (in a given distance), in a subnation: 
//it is used to determine near cells of an individual in order to understand if he/her is near infected people.
struct arrayWithSize findRectangle(struct subnation subnationItem,
	struct individual* p, int distanceToBeInfected, int rank, int w, int l) {
	struct arrayWithSize  r;
	r.currentSize = 0;
	r.maxSize = 0;
	r.pList = NULL;

	for (int i = -distanceToBeInfected; i < distanceToBeInfected; i++)
	{
		for (int j = -distanceToBeInfected; j < distanceToBeInfected; j++)
		{
			int r3 = rectIndex(p->position.x + j, p->position.y + i, w, l);

			if (r3 >= 0 && r3 < subnationItem.nRectangles)
			{
				r = insertInt(r, r3);
			}
		}
	}

	return r;
}


//method that returns all the people inside a rectangle/cell, given its index
struct arrayWithSize getPeopleNear(int rectIndex, struct subnation subnationItem, int w, int l) {
	struct arrayWithSize r;
	r.currentSize = 0;
	r.maxSize = 0;
	r.pList = NULL;

	struct point p;
	p.x = rectIndex / w;
	p.y = rectIndex % l;

	struct individual** p2 = subnationItem.people.pList;
	for (int i = 0; i < subnationItem.people.currentSize; i++)
	{
		if (p2[i]->position.x == p.x && p2[i]->position.y == p.y)
		{
			r = insertIndividual(r, p2[i]);
		}
	}

	if (r.currentSize == 0)
	{
		int a = 0;
		a++;
	}

	return r;
}

int dimHash = 100;

void putHash(struct arrayWithSize storicoContatti, struct individual* p) {
	int i = (p->id) % dimHash;
	struct arrayWithSize* t1 = storicoContatti.pList;
	struct arrayWithSize t2 = t1[i];
	t2 = insertHashIndividual(t2, p);
	t1[i] = t2;
}

struct arrayWithSizeAndIndividual* getHash(struct arrayWithSize storicoContatti, struct individual* p) {
	int i = (p->id) % dimHash;
	struct arrayWithSize* t1 = storicoContatti.pList;
	struct arrayWithSize t2 = t1[i];
	struct arrayWithSizeAndIndividual* t3 = t2.pList;
	for (int j = 0; j < t2.currentSize; j++)
	{
		struct arrayWithSizeAndIndividual t4 = t3[j];
		if (t4.i->id == p->id && t4.i->rank == p->rank)
		{
			return &(t3[j]);
		}
	}

	return NULL;
}

//method that finds the contact history of an individual
struct contactHistory* FindContactHistory(struct individual* p, struct individual* p2, struct arrayWithSize storicoContatti) {
	struct arrayWithSizeAndIndividual* t1 = getHash(storicoContatti, p);

	if (t1 == NULL)
	{
		return NULL;
	}

	struct contactHistory** t3 = t1->a.pList;
	for (int i = 0; i < t1->a.currentSize; i++)
	{
		if (t3[i]->from->id == p->id && t3[i]->from->rank == p->rank
			&& t3[i]->to->id == p2->id && t3[i]->to->rank == p2->rank)
		{
			return t3[i];
		}
	}

	return NULL;
}



struct individualSummaryWithRank*
	calculateVirus2(
		struct individualSummaryWithRank* buffer,
		struct subnation subNazioneItem, int rank,
		int t, int distanceToBeInfected, struct arrayWithSize people,
		struct arrayWithSize storicoContatti,
		int w, int l, int i_t2, int subnazioneIndex, int velocity)
{
	struct individual** plist = people.pList;

	if (buffer[subnazioneIndex].individualSummary.infected > 0) { //only if the subnation has infected people we need to calculate the evolution, otherwise it's useless

		//for each person in the subnation
		for (int ip = 0; ip < people.currentSize; ip++)
		{
			if (plist[ip]->subnation != subnazioneIndex)
				continue;

			if (t * i_t2 >= plist[ip]->lastTimeHeWasInfected + minTimeToHeal && plist[ip]->isInfected == 1) { //if he/her is infected and it's time become healthier again
				plist[ip]->isInfected = 0;
				buffer[subnazioneIndex].individualSummary.infected--;
				buffer[subnazioneIndex].individualSummary.sane++;
				plist[ip]->lastTimeHeRecovered = t * i_t2;
			}

			if (buffer[subnazioneIndex].individualSummary.infected > 0) { //only if the subnation has infected people we need to calculate the evolution, otherwise it's useless
				struct arrayWithSize r = findRectangle(subNazioneItem, plist[ip], distanceToBeInfected, rank, w, l); //rectangles near him/her
				int* r2 = r.pList;
				if (r.currentSize > 0) {
					for (int i = 0, rSize = r.currentSize; i < rSize; i++) { //for each rectangle/cell near him/her
						struct arrayWithSize peopleNear = getPeopleNear(r2[i], subNazioneItem, w, l); //Get people in that rectangle
						struct individual** plist2 = peopleNear.pList;

						for (int ip2 = 0; ip2 < peopleNear.currentSize; ip2++) { //for each people in that rectangle
		
							struct individual* pc1 = (plist[ip]);
							struct individual* pc2 = (plist2[ip2]);

							if ((pc1->id != pc2->id && pc1->rank == pc2->rank) && pc2->isInfected) //if the person we have been near is infected
							{
								struct contactHistory* vicinanzaItem = FindContactHistory(pc1, pc2, storicoContatti);
								if (vicinanzaItem == NULL || vicinanzaItem->to == NULL && vicinanzaItem->from == NULL) {

									//first time we encounter this person

									struct contactHistory* v = malloc(sizeof(struct contactHistory));
									v->from = pc1;
									v->timeEnd = t * i_t2;
									v->timeStart = t * i_t2;
									v->to = pc2;

									struct arrayWithSizeAndIndividual* v2 = getHash(storicoContatti, pc1);

									//insert contact history with this person
									if (v2 != NULL) {
			
										v2 = insertContactHistory(v2, v);
									}
									else {
										putHash(storicoContatti, pc1);


										v2 = getHash(storicoContatti, pc1);

										v2 = insertContactHistory(v2, v);
									}

									//if we stayed near him/her too much, we become infected
									if (v->timeEnd - v->timeStart >= minTimeToGetInfected
										&& !pc1->isInfected && (pc1->lastTimeHeRecovered < 0 || pc1->lastTimeHeRecovered + TimeToBecomeSuspceptibleAgain >= (t * i_t2)))
									{
										pc1->isInfected = 1;
										buffer[subnazioneIndex].individualSummary.sane--;
										buffer[subnazioneIndex].individualSummary.infected++;
										pc1->lastTimeHeWasInfected = t * i_t2;
									}
								}
								else {

									//we have encountered this person before

									vicinanzaItem->timeEnd = t * i_t2;

									//if we stayed near him/her too much, we become infected
									if (vicinanzaItem->timeEnd - vicinanzaItem->timeStart >= minTimeToGetInfected
										&& !pc1->isInfected && (pc1->lastTimeHeRecovered < 0 || pc1->lastTimeHeRecovered + TimeToBecomeSuspceptibleAgain >= (t * i_t2))) {
										pc1->isInfected = 1;
										buffer[subnazioneIndex].individualSummary.sane--;
										buffer[subnazioneIndex].individualSummary.infected++;
										pc1->lastTimeHeWasInfected = t * i_t2;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	//let's move all people to a random position, based on their velocity
	for (int i = 0; i < people.currentSize; i++)
	{
		int moveX = getRandomNumber(0, 1);
		int moveY = getRandomNumber(0, 1);

		if (moveX == 0)
			plist[i]->position.x += velocity;
		else
			plist[i]->position.x -= velocity;

		if (moveY == 0)
			plist[i]->position.y += velocity;
		else
			plist[i]->position.y -= velocity;

		if (plist[i]->position.x >= w)
		{
			plist[i]->position.x = w - 1;
		}

		if (plist[i]->position.x < 0)
		{
			plist[i]->position.x = 0;
		}

		if (plist[i]->position.y >= l)
		{
			plist[i]->position.y = l - 1;
		}

		if (plist[i]->position.y < 0)
		{
			plist[i]->position.y = 0;
		}
	}

	return buffer;
}

struct individualSummaryWithRank* calculateVirus1(
	struct individualSummaryWithRank* buffer,
	struct nation nationItem,
	int rank, int t, int distanceToBeInfected,
	struct arrayWithSize people,
	struct arrayWithSize storicoContatti,
	int w, int l, int velocity)
{

	printf("\n");

	//calculate how many "timesteps" we need to do in a day, based on the timestamp "t".
	int t2 = 60 * 60 * 24 / t;

	struct arrayWithSize subnazioneArrayList = GetSubnations(nationItem, rank);
	struct subnation* p2 = subnazioneArrayList.pList;

	//for each subnation
	for (int sbi = 0; sbi < subnazioneArrayList.currentSize; sbi++) { 
		struct subnation sb2 = p2[sbi];

		//for each timestep in a day
		for (int i = 0; i < t2; i++) {
			buffer = calculateVirus2(buffer, sb2,
				rank, t, distanceToBeInfected, people,
				storicoContatti, w, l, i, sbi, velocity);


		}
	}

	return buffer;
}

void printArrayInt(struct arrayWithSize a) {
	int* b = a.pList;
	printf("Array, size %d, content: ", a.currentSize);
	for (int i = 0; i < a.currentSize; i++) {
		printf("%d ", b[i]);
	}
	printf("\n");
}

int main(int argc, char** argv) {
	// Init random number generator
	srand((unsigned int)time(NULL));

	MPI_Init(NULL, NULL);

	int my_rank, world_size;
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	//Setup variables to default values
	int numInfected = 500;// 100;
	int numPeople = 500; //500;
	struct point dimWorld;
	dimWorld.x = 25;// 250;
	dimWorld.y = 25; //250;
	int days = 3; //5;
	struct point dimSubNation;
	dimSubNation.x = 5;//125;
	dimSubNation.y = 5;//125;
	int timeStep = 10 * 60;
	int distanceToBeInfected = 1;//10;
	int velocity = 1;

	if (argc < 11) {
		if (my_rank == 0)
			printf("mpiexec -n WorldSize executable_path numInfectedTotal numPeopleTotal dimWorldX dimWorldY days dimSubNationX dimSubNationY timestep distance velocity\n\n");

		MPI_Finalize();
		return;
	}

	numInfected = atoi(argv[1]);
	numPeople = atoi(argv[2]);
	dimWorld.x = atoi(argv[3]);
	dimWorld.y = atoi(argv[4]);
	days = atoi(argv[5]);
	dimSubNation.x = atoi(argv[6]);
	dimSubNation.y = atoi(argv[7]);
	timeStep = atoi(argv[8]);
	distanceToBeInfected = atoi(argv[9]);
	velocity = atoi(argv[10]);


	if (numPeople <= 0)
	{
		if (my_rank == 0)
			printf("The number of people must be a positive number (>0) \n\n");

		MPI_Finalize();
		return;
	}

	if (numPeople < numInfected)
	{
		if (my_rank == 0)
			printf("The total number of people can't be lower than total number of infected \n\n");

		MPI_Finalize();
		return;
	}

	if (dimSubNation.x > dimWorld.x)
	{
		if (my_rank == 0)
			printf("The X dimension of the subnation can't be greater than the dimension of the world \n\n");

		MPI_Finalize();
		return;
	}

	if (dimSubNation.y > dimWorld.y)
	{
		if (my_rank == 0)
			printf("The Y dimension of the subnation can't be greater than the dimension of the world \n\n");

		MPI_Finalize();
		return;
	}

	if (distanceToBeInfected < 0)
	{
		if (my_rank == 0)
			printf("The distance to be infected must be a positive number (>=0) \n\n");

		MPI_Finalize();
		return;
	}

	if (days <= 0)
	{
		if (my_rank == 0)
			printf("The days must be a positive number (>0) \n\n");

		MPI_Finalize();
		return;
	}

	if (velocity <= 0)
	{
		if (my_rank == 0)
			printf("The velocity must be a positive number (>0) \n\n");

		MPI_Finalize();
		return;
	}


	if (timeStep <= 0)
	{
		if (my_rank == 0)
			printf("The timestep must be a positive number (>0) \n\n");

		MPI_Finalize();
		return;
	}

	int subNationsNum = calculateNumSubnations(dimWorld.x, dimWorld.y, dimSubNation.x, dimSubNation.y); //calculate how many subnations the world will be divided into
	int sizeOfIndividualSummaryWithRank = sizeof(struct individualSummaryWithRank);
	if (subNationsNum < 0)
	{
		MPI_Finalize();
		return;
	}

	MPI_Barrier(MPI_COMM_WORLD);

	//calculate how much subnations each mpi process will get
	struct arrayWithSize howManySubNationsPerProcess =
		calculateDistributionSubnationsToProcess(world_size, subNationsNum);


	int maxSubnationPerProcess = getMax(howManySubNationsPerProcess);
	if (maxSubnationPerProcess < 0)
	{
		MPI_Finalize();
		return;
	}


	MPI_Barrier(MPI_COMM_WORLD);

	struct individualSummaryWithRank* start = NULL; //global_arr
	struct individualSummaryWithRank* buffer = NULL;
	struct individualSummaryWithRank* buffer3 = NULL;
	if (my_rank == 0)
	{

		//generate how many infected/sane each subnation will get
		start = fillPeopleInformation(numPeople, numInfected, world_size,
			howManySubNationsPerProcess, maxSubnationPerProcess);

		if (start == NULL)
		{
			printf("start=NULL \n");
			MPI_Finalize();
			return;
		}

		buffer3 = malloc(sizeof(struct individualSummaryWithRank) * world_size * maxSubnationPerProcess);
	}

	buffer = malloc(sizeof(struct individualSummaryWithRank) * maxSubnationPerProcess);


	// Scatter the random numbers from process 0 to all processes
	int scale = sizeof(struct individualSummaryWithRank) / sizeof(int);
	int dimScatter = maxSubnationPerProcess * scale;
	MPI_Scatter(start, dimScatter, MPI_INT,
		buffer, dimScatter, MPI_INT,
		0, MPI_COMM_WORLD);

	struct nation nationItem;

	//each mpi process generates its nation map and populates its people into the nation/subnations
	nationItem = GenerateMap(my_rank, howManySubNationsPerProcess,
		dimSubNation.x, dimSubNation.y, buffer, maxSubnationPerProcess);

	//get all people in the nation
	struct arrayWithSize people = getPeople(nationItem);

	struct arrayWithSize hashHistoryVar;
	hashHistoryVar.currentSize = dimHash;
	hashHistoryVar.maxSize = hashHistoryVar.currentSize;
	hashHistoryVar.pList = (struct arrayWithSize*)malloc(sizeof(struct arrayWithSize) * hashHistoryVar.maxSize);
	struct arrayWithSize* sb3 = hashHistoryVar.pList;
	for (int i = 0; i < dimHash; i++)
	{
		sb3[i].currentSize = 0;
		sb3[i].maxSize = 0;
		sb3[i].pList = NULL;
	}

	//for each day
	for (int i = -1; i < days; i++)
	{
		if (i >= 0) //we use this to print info "at the day of the day 0", so at the start of the run
		{
			buffer = calculateVirus1(buffer, nationItem,
				my_rank, timeStep, distanceToBeInfected, people, hashHistoryVar, dimSubNation.x, dimSubNation.y, velocity);
		}

		MPI_Gather(buffer, maxSubnationPerProcess * scale, MPI_INT,
			buffer3, maxSubnationPerProcess * scale, MPI_INT,
			0, MPI_COMM_WORLD);

		//main mpi process prints results
		if (my_rank == 0)
		{
			printf("End of the day [%d]:\n", (i + 1));

			struct arrayWithSize buffer3_toprint;
			buffer3_toprint.pList = buffer3;
			buffer3_toprint.currentSize = maxSubnationPerProcess * world_size;
			buffer3_toprint.maxSize = buffer3_toprint.currentSize;
			printArray(buffer3_toprint, maxSubnationPerProcess);
		}

		MPI_Barrier(MPI_COMM_WORLD);
	}


	free2(start);
	free2(buffer3);
	free2(buffer);
	free3(howManySubNationsPerProcess);

	struct arrayWithSize* a = hashHistoryVar.pList;
	for (int i = 0; i < dimHash; i++)
	{
		struct arrayWithSizeAndIndividual* b = a[i].pList;
		for (int j = 0; j < a[i].currentSize; j++)
		{
			free3(b[j].a);
		}
	}
	free3(hashHistoryVar);

	free3(people);
	free4(nationItem);

	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();
}