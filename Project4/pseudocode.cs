class individualSummary{
    int sani;
    int infetti;
    int rank;
}


class vicinanza{
    int timeInizio;
    int timeFine;
    person from;
}

class person {
    int rank;
    point p;
    bool inInfected;
}

class point{
    int x;
    int y;
}

void riempi(ref a, int numPeople, int numInfected, int numProcess)
{

}

int? calculateRectangles(int W, int L, int w, int l)
{
    return (W*L)/(w*l);
}

void main(){

    int numProcess; //world_size
    int my_rank;
    int numInfected;
    int numPeople;
    int days;
    int W,L;
    int w,l;
    int d; //distance to get infected
    int rectangles = calculateRectangles(W,L,w,l);
    if (rectangles == null)
    {
        if (my_rank == 0)
        {
            printf("Le aree non sono ben suddivisibili!");
        }
        return;
    }

    int howManyRectanglesPerProcess = calculateDistributionRectanglesToProcess(numProcess, rectangles);

    individualSummary start[howManyRectanglesPerProcess * rectangles];
    individualSummary buffer[howManyRectanglesPerProcess];
    individualSummary buffer2[howManyRectanglesPerProcess];
    individualSummary buffer3[howManyRectanglesPerProcess * rectangles];

    if (my_rank == 0)
    {
       riempi(a, numPeople, numInfected, numProcess, howManyRectanglesPerProcess); 
    }

    mpi_scatter(
        source: start, 
        dest: buffer,
        num: howManyRectanglesPerProcess,
        tag: 0
    );

    for (int i=0; i<days; i++)
    {
        if (my_rank != 0)
        {
            calcola(buffer, buffer2);
        }

        mpi_gather(
            source: buffer2,
            dest: buffer3,
            num: howManyRectanglesPerProcess,
            tag: (i+1)
        );

        if (my_rank == 0)
        {
            printArray(buffer3);
        }
        else
        {
            buffer = buffer2;
        }

        mpi_Barrier();

    }


}