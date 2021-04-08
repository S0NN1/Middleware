#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>
#include <math.h>

struct point {
	int x;
	int y;
};

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
	int id;
	int subnation;
};

struct vicinanza {
	int timeInizio;
	int timeFine;
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

struct arrayWithSize calculateDistributionSubnationsToProcess(int numProcess, int numSubnations) {
	/*
	if (rectangles<=numProcess){
		int[] r = new int[numProcess];
		for(int i=0; i<numProcess; i++)
		{
			if (i<rectangles)
			{
				r[i] = 1;
			}
			else{
				r[i] = 0;
			}
		}

		return r;
	}

	int ratio = rectangles / numProcess;
	int b = ratio * (numProcess - 1);
	int c = rectangles - b;
	int[] r = new int[numProcess];
	for (int i=0; i<numProcess - 1; i++)
	{
		r[i] = ratio;
	}

	r[numProcess-1] = c;
	return r;
	*/

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

struct individualSummaryWithRank* riempi(int numPeople, int numInfected,
	int numProcess, struct arrayWithSize maxRectanglesForEachProcess, int maxRectanglesPerProcess) {
	int sani = numPeople - numInfected;
	if (maxRectanglesPerProcess < 0)
		return NULL;

	struct individualSummaryWithRank* returnValue =
		malloc(sizeof(struct individualSummaryWithRank) * numProcess * maxRectanglesPerProcess);

	for (int i = 0; i < numProcess; i++) {
		for (int j = 0; j < maxRectanglesPerProcess; j++)
		{
			int k = (i * maxRectanglesPerProcess) + j;
			returnValue[k].rank = (i * maxRectanglesPerProcess) + j;
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
		int k = (where1 * maxRectanglesPerProcess) + where2;
		returnValue[k].individualSummary.infected++;
	}

	for (int i = 0; i < sani; i++)
	{
		int where1 = -1;
		do {
			where1 = getRandomNumber(0, numProcess - 1);
		} while (m2[where1] - 1 < 0);
		int where2 = getRandomNumber(0, m2[where1] - 1);
		int k = (where1 * maxRectanglesPerProcess) + where2;
		returnValue[k].individualSummary.sane++;
	}

	return returnValue;
}

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

struct arrayWithSizeAndIndividual* insertVicinanza(struct arrayWithSizeAndIndividual* arrayWithSize, struct vicinanza* vic) {
	while (1)
	{
		struct vicinanza** r = arrayWithSize->a.pList;

		if (arrayWithSize->a.currentSize < arrayWithSize->a.maxSize)
		{
			r[arrayWithSize->a.currentSize] = vic;
			arrayWithSize->a.currentSize++;
			return arrayWithSize;
		}

		int newSize = arrayWithSize->a.maxSize * 2;
		if (newSize < 1)
			newSize = 1;

		struct vicinanza** r2 = malloc(sizeof(vic) * newSize);
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

struct nation GeneraMappa(
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

		//r.rettangoli = GeneraRettangoli(w,l);
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
			personItem->id = r.people.currentSize;
			personItem->subnation = k2;
			r.people = insertIndividual(r.people, personItem);
		}

		n.list = insertSubnation(n.list, r);
	}

	return n;
}

struct arrayWithSize getPeople(struct nation nazioneItem) {
	struct arrayWithSize people;
	people.currentSize = 0;
	people.maxSize = 0;
	people.pList = NULL;

	struct subnation* p2 = nazioneItem.list.pList;
	for (int i = 0; i < nazioneItem.list.currentSize; i++)
	{
		struct individual** p3 = p2[i].people.pList;
		for (int j = 0; j < p2[i].people.currentSize; j++)
		{
			people = insertIndividual(people, p3[j]);
		}
	}

	return people;
}

void printInfoInfetti(struct individualSummaryWithRank i2, int maxRectanglesPerProcess, int i) {
	if (maxRectanglesPerProcess == 1)
	{
		printf("  >TOT: INFETTI %d, SANI %d\n", i2.individualSummary.infected, i2.individualSummary.sane);
	}
	else {
		int index = (i2.rank - ((i / maxRectanglesPerProcess) * maxRectanglesPerProcess));
		if (index >= 10)
			printf("  > %d: INFETTI %d, SANI %d\n", index, i2.individualSummary.infected, i2.individualSummary.sane);
		else
			printf("  >  %d: INFETTI %d, SANI %d\n", index, i2.individualSummary.infected, i2.individualSummary.sane);
	}
}

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

		printInfoInfetti(p3, maxRectanglesPerProcess, i);

		if (maxRectanglesPerProcess > 0)
		{
			if ((i + 1) % maxRectanglesPerProcess == 0)
			{
				printf("  >TOT: INFETTI %d, SANI %d\n", (totInfectedPartial), (totSanePartial));
			}
		}
	}

	printf(">TOT: INFETTI %d, SANI %d\n", (totInfectedTotal), (totSaneTotal));
}

struct arrayWithSize GetSubnazioni(struct nation nazioneItem, int rank) {
	struct arrayWithSize r;
	r.currentSize = 0;
	r.maxSize = 0;
	r.pList = NULL;

	struct subnation* p2 = nazioneItem.list.pList;
	for (int i = 0; i < nazioneItem.list.currentSize; i++)
	{
		if (p2[i].rank == rank)
		{
			r = insertSubnation(r, p2[i]);
		}
	}

	return r;
}

int minDurataPerInfettarsi = 10 * 60;
int durataPerGuarire = 15 * 60;

int rectIndex(int x, int y, int w, int l) {
	return (x * w) + y;
}

struct arrayWithSize trovaRettangolo(struct subnation subnazioneItem,
	struct individual* p, int d, int rank, int w, int l) {
	struct arrayWithSize  r;
	r.currentSize = 0;
	r.maxSize = 0;
	r.pList = NULL;

	for (int i = -d; i < d; i++)
	{
		for (int j = -d; j < d; j++)
		{
			int r3 = rectIndex(p->position.x + j, p->position.y + i, w, l);
			//rettangolo r2 = trovaRettangolo2(subnazioneItem, p.p.x + j, p.p.y + i, rank);
			if (r3 >= 0 && r3 < subnazioneItem.nRectangles)
			{
				r = insertInt(r, r3);
			}
		}
	}

	return r;
}

struct arrayWithSize getPeopleNear(int rectIndex, struct subnation subnazioneItem, int w, int l) {
	struct arrayWithSize r;
	r.currentSize = 0;
	r.maxSize = 0;
	r.pList = NULL;

	struct point p;
	p.x = rectIndex / w;
	p.y = rectIndex % l;

	struct individual** p2 = subnazioneItem.people.pList;
	for (int i = 0; i < subnazioneItem.people.currentSize; i++)
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

	/*
	for (var x : subnazioneItem.people) {
		if (x.p.equals(p))
		{
			r.add(x);
		}
	}
	*/

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

struct vicinanza* TrovaVicinanza(struct individual* p, struct individual* p2, struct arrayWithSize storicoContatti) {
	struct arrayWithSizeAndIndividual* t1 = getHash(storicoContatti, p);
	//var t1 = storicoContatti.get(p);

	if (t1 == NULL)
	{
		return NULL;
	}

	struct vicinanza** t3 = t1->a.pList;
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
	calcolaAvanzamentoContagi2(
		struct individualSummaryWithRank* buffer,
		struct subnation subNazioneItem, int rank,
		int t, int d, struct arrayWithSize people,
		struct arrayWithSize storicoContatti,
		int w, int l, int i_t2, int subnazioneIndex)
{
	struct individual** plist = people.pList;

	if (buffer[subnazioneIndex].individualSummary.infected > 0) {
		for (int ip = 0; ip < people.currentSize; ip++)
		{
			if (plist[ip]->subnation != subnazioneIndex)
				continue;

			if (t * i_t2 >= plist[ip]->lastTimeHeWasInfected + durataPerGuarire && plist[ip]->isInfected == 1) {
				plist[ip]->isInfected = 0;
				buffer[subnazioneIndex].individualSummary.infected--;
				buffer[subnazioneIndex].individualSummary.sane++;
			}

			if (buffer[subnazioneIndex].individualSummary.infected > 0) {
				struct arrayWithSize r = trovaRettangolo(subNazioneItem, plist[ip], d, rank, w, l);
				int* r2 = r.pList;
				if (r.currentSize > 0) {
					for (int i = 0, rSize = r.currentSize; i < rSize; i++) {
						struct arrayWithSize peopleNear = getPeopleNear(r2[i], subNazioneItem, w, l);
						struct individual** plist2 = peopleNear.pList;

						for (int ip2 = 0; ip2 < peopleNear.currentSize; ip2++) {
							//for (person p2 : peopleNear) {
							struct individual* pc1 = (plist[ip]);
							struct individual* pc2 = (plist2[ip2]);

							if ((pc1->id != pc2->id && pc1->rank == pc2->rank) && pc2->isInfected)
							{
								struct vicinanza* vicinanzaItem = TrovaVicinanza(pc1, pc2, storicoContatti);
								if (vicinanzaItem == NULL || vicinanzaItem->to == NULL && vicinanzaItem->from == NULL) {
									struct vicinanza* v = malloc(sizeof(struct vicinanza));
									v->from = pc1;
									v->timeFine = t * i_t2;
									v->timeInizio = t * i_t2;
									v->to = pc2;

									struct arrayWithSizeAndIndividual* v2 = getHash(storicoContatti, pc1);
									//struct arrayWithSize v2 = storicoContatti.get(pc1);

									if (v2 != NULL) {
										//v2.add(v);
										v2 = insertVicinanza(v2, v);
									}
									else {
										putHash(storicoContatti, pc1);
										//storicoContatti.put(pc1, new ArrayList<>());

										v2 = getHash(storicoContatti, pc1);
										//v2.add(v);
										v2 = insertVicinanza(v2, v);
									}

									if (v->timeFine - v->timeInizio >= minDurataPerInfettarsi
										&& !pc1->isInfected)
									{
										pc1->isInfected = 1;
										buffer[subnazioneIndex].individualSummary.sane--;
										buffer[subnazioneIndex].individualSummary.infected++;
										pc1->lastTimeHeWasInfected = t * i_t2;
									}
								}
								else {
									vicinanzaItem->timeFine = t * i_t2;

									if (vicinanzaItem->timeFine - vicinanzaItem->timeInizio >= minDurataPerInfettarsi
										&& !pc1->isInfected) {
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

	for (int i = 0; i < people.currentSize; i++)
	{
		int moveX = getRandomNumber(0, 1);
		int moveY = getRandomNumber(0, 1);

		if (moveX == 0)
			plist[i]->position.x++;
		else
			plist[i]->position.x--;

		if (moveY == 0)
			plist[i]->position.y++;
		else
			plist[i]->position.y--;

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

struct individualSummaryWithRank* calcolaAvanzamentoContagi(
	struct individualSummaryWithRank* buffer,
	struct nation nazioneItem,
	int rank, int t, int d,
	struct arrayWithSize people,
	struct arrayWithSize storicoContatti,
	int w, int l)
{
	//struct individualSummary* a = buffer[rank];

	printf("\n");

	int t2 = 60 * 60 * 24 / t;

	struct arrayWithSize subnazioneArrayList = GetSubnazioni(nazioneItem, rank);
	struct subnation* p2 = subnazioneArrayList.pList;
	for (int sbi = 0; sbi < subnazioneArrayList.currentSize; sbi++) {
		struct subnation sb2 = p2[sbi];

		for (int i = 0; i < t2; i++) {
			buffer = calcolaAvanzamentoContagi2(buffer, sb2,
				rank, t, d, people,
				storicoContatti, w, l, i, sbi);

			//printArray(buffer, rank);
			//System.out.println("");
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

	int numInfected = 500;// 100;
	int numPeople = 500; //500;
	struct point dimWorld;
	dimWorld.x = 25;// 250;
	dimWorld.y = 25; //250;
	int days = 3; //5;
	struct point dimSubCountry;
	dimSubCountry.x = 5;//125;
	dimSubCountry.y = 5;//125;
	int timeStep = 10 * 60;
	int d = 1;//10;

	if (argc < 10) {
		printf("mpiexec -n WorldSize .\exec numInfectedTotal numPeopleTotal dimWorldX dimWorldY days dimSubCountryX dimSubCountryY timestep distance\n\n");
		return;
	}

	numInfected = atoi(argv[1]);
	numPeople = atoi(argv[2]);
	dimWorld.x = atoi(argv[3]);
	dimWorld.y = atoi(argv[4]);
	days = atoi(argv[5]);
	dimSubCountry.x = atoi(argv[6]);
	dimSubCountry.y = atoi(argv[7]);
	timeStep = atoi(argv[8]);
	d = atoi(argv[9]);

	int subNationsNum = calculateNumSubnations(dimWorld.x, dimWorld.y, dimSubCountry.x, dimSubCountry.y);
	int sizeOfIndividualSummaryWithRank = sizeof(struct individualSummaryWithRank);
	if (subNationsNum < 0)
	{
		//printf("Le aree non sono ben suddivisibili \n");
		MPI_Finalize();
		return;
	}
	else {
		//printf("Le aree sono ben suddivisibili: %d \n", subNationsNum);
	}

	MPI_Init(NULL, NULL);

	int my_rank, world_size;
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	//printf("Hi. I'm alive. My rank is %d and world size is %d \n", my_rank, world_size);
	MPI_Barrier(MPI_COMM_WORLD);

	struct arrayWithSize howManySubNationsPerProcess =
		calculateDistributionSubnationsToProcess(world_size, subNationsNum);

	//printArrayInt(howManySubNationsPerProcess);

	int maxRectanglesPerProcess = getMax(howManySubNationsPerProcess);
	if (maxRectanglesPerProcess < 0)
	{
		//printf("maxRectanglesPerProcess<0");
		MPI_Finalize();
		return;
	}
	else
	{
		//printf("maxRectanglesPerProcess=%d \n", maxRectanglesPerProcess);
	}

	MPI_Barrier(MPI_COMM_WORLD);

	struct individualSummaryWithRank* start = NULL; //global_arr
	struct individualSummaryWithRank* buffer = NULL;
	struct individualSummaryWithRank* buffer3 = NULL;
	if (my_rank == 0)
	{
		start = riempi(numPeople, numInfected, world_size,
			howManySubNationsPerProcess, maxRectanglesPerProcess);

		if (start == NULL)
		{
			printf("start=NULL \n");
			MPI_Finalize();
			return;
		}

		buffer3 = malloc(sizeof(struct individualSummaryWithRank) * world_size * maxRectanglesPerProcess);
	}

	buffer = malloc(sizeof(struct individualSummaryWithRank) * maxRectanglesPerProcess);
	//buffer2 = malloc(sizeof(struct individualSummaryWithRank) * maxRectanglesPerProcess);

// Scatter the random numbers from process 0 to all processes
	int scale = sizeof(struct individualSummaryWithRank) / sizeof(int);
	int dimScatter = maxRectanglesPerProcess * scale;
	MPI_Scatter(start, dimScatter, MPI_INT,
		buffer, dimScatter, MPI_INT,
		0, MPI_COMM_WORLD);

	if (buffer != NULL)
	{
		//printf("Hi I'm process %d and I will now print the first element of my received buffer. Infected: %d, sane: %d, rank: %d \n",
		//	my_rank, buffer[0].individualSummary.infected, buffer[0].individualSummary.sane, buffer[0].rank);
	}

	struct nation nazioneItem;

	nazioneItem = GeneraMappa(my_rank, howManySubNationsPerProcess,
		dimSubCountry.x, dimSubCountry.y, buffer, maxRectanglesPerProcess);

	struct subnation* sb1 = nazioneItem.list.pList;
	if (sb1 == NULL)
	{
		//printf("Hi I'm process %d and I the first element of my nation is null\n", my_rank);
	}
	else {
		struct individual** sb2 = sb1[0].people.pList;

		//printf("Hi I'm process %d and I will now print the first element of my nation. RankNation: %d, size: %d, rectangles: %d, rankSubnation: %d, Xposition: %d, Yposition: %d, IdFirstPerson: %d \n",
		//	my_rank, nazioneItem.rank, nazioneItem.list.currentSize, sb1[0].nRectangles, sb1[0].rank, sb1[0].position.x, sb1[0].position.y, sb2[0]->id);
	}

	struct arrayWithSize people = getPeople(nazioneItem);

	struct arrayWithSize hashStoricoVar;
	hashStoricoVar.currentSize = dimHash;
	hashStoricoVar.maxSize = hashStoricoVar.currentSize;
	hashStoricoVar.pList = (struct arrayWithSize*)malloc(sizeof(struct arrayWithSize) * hashStoricoVar.maxSize);
	struct arrayWithSize* sb3 = hashStoricoVar.pList;
	for (int i = 0; i < dimHash; i++)
	{
		sb3[i].currentSize = 0;
		sb3[i].maxSize = 0;
		sb3[i].pList = NULL;
	}

	for (int i = -1; i < days; i++)
	{
		if (i >= 0)
		{
			buffer = calcolaAvanzamentoContagi(buffer, nazioneItem,
				my_rank, timeStep, d, people, hashStoricoVar, dimSubCountry.x, dimSubCountry.y);
		}

		MPI_Gather(buffer, maxRectanglesPerProcess * scale, MPI_INT,
			buffer3, maxRectanglesPerProcess * scale, MPI_INT,
			0, MPI_COMM_WORLD);

		if (my_rank == 0)
		{
			printf("End of the day [%d]:\n", (i + 1));

			struct arrayWithSize buffer3_toprint;
			buffer3_toprint.pList = buffer3;
			buffer3_toprint.currentSize = maxRectanglesPerProcess * world_size;
			buffer3_toprint.maxSize = buffer3_toprint.currentSize;
			printArray(buffer3_toprint, maxRectanglesPerProcess);
		}

		MPI_Barrier(MPI_COMM_WORLD);
	}

	// Clean up
	if (my_rank == 0) {
		free2(start);
		free2(buffer3);
	}

	free2(buffer);
	//free2(buffer2);

	free3(howManySubNationsPerProcess);

	struct arrayWithSize* a = hashStoricoVar.pList;
	for (int i = 0; i < dimHash; i++)
	{
		struct arrayWithSizeAndIndividual* b = a[i].pList;
		for (int j = 0; j < a[i].currentSize; j++)
		{
			free3(b[j].a);
		}
	}

	free3(people);
	free4(nazioneItem);

	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();
}