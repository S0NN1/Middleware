package com.company;

import java.util.*;

class Nullable<T>{

    public T item;

    Nullable(T item)
    {
        this.item = item;
    }
}


public class Main {


    public static int getRandomNumber(int min, int max) {
        return (int) ((Math.random() * (max - min)) + min);
    }

    public static void main(String[] args){
        for (int i=0; i<1; i++) {
            final Integer i2 = i;
            Thread thread = new Thread(new Runnable() {
                @Override
                public void run() {
                    int my_rank = i2;
                    int numPeople = 500;
                    int d = 2; //distance to get infected
                    point dimWorld = new point();
                    dimWorld.x = 250;
                    dimWorld.y = 250;
                    point dimSubCountry = new point();
                    dimSubCountry.x = 150;
                    dimSubCountry.y = 150;
                    int days = 5;
                    int numProcess = 11;
                    int timeStep = 30 * 60;

                    main2(my_rank, numPeople, numProcess, timeStep, (int)(numPeople*0.3d),
                            d, dimWorld, dimSubCountry, days);
                }
            });
            thread.start();
        }

    }

    static Nullable<Integer> calculateNumSubnations(int W, int L, int w, int l)
    {
        float f = (float)W*L;
        f /= (w*l);

        int i = W*L;
        i /= (w*l);

        if (f == i) {
            int n = (W*L)/(w*l);
            return new Nullable<Integer>((int)Math.sqrt(n));
        }

        return null;
    }

    public static void main2(int my_rank, int numPeople, int numProcess,
                             int t, int numInfected, int d, point world, point dimWorld, int days)
    {
	// write your code here


        int W = dimWorld.x,L = dimWorld.y;
        int w = dimWorld.x, l = dimWorld.y;

        Nullable<Integer> subNationsNum = calculateNumSubnations(W,L,w,l);

        if (subNationsNum == null)
        {
            if (my_rank == 0)
            {
                System.console().writer().println("Le aree non sono ben suddivisibili!");
            }
            return;
        }

        int[] howManySubNationsPerProcess = calculateDistributionSubnationsToProcess(numProcess, subNationsNum.item);
        int maxRectanglesPerProcess = Arrays.stream(howManySubNationsPerProcess).max().getAsInt();

        individualSummary[][] start;
        start = new individualSummary[numProcess][maxRectanglesPerProcess];
        individualSummary[][] buffer;
        buffer = new individualSummary[1][maxRectanglesPerProcess];
        individualSummary[][] buffer2;
        buffer2 = new individualSummary[1][maxRectanglesPerProcess];
        individualSummary[][] buffer3;
        buffer3 = new individualSummary[numProcess][maxRectanglesPerProcess];

        if (my_rank == 0 || true) //todo: remove "|| true"
        {
            start = riempi(start, numPeople, numInfected, numProcess, howManySubNationsPerProcess);
        }

        buffer = mpi_scatter(
                 start,
                 buffer,
                 -1,
                 0
        );

        nazione nazioneItem = null;
        
        if (my_rank != 0 || true) //todo: remove "|| true"
        {
            nazioneItem = GeneraMappa(my_rank, howManySubNationsPerProcess, w, l, start);
        }


        ArrayList<person> people = getPeople(nazioneItem);
        Dictionary<person, ArrayList<vicinanza>> storicoContatti = new Hashtable<>();

        for (int i=0; i<days; i++)
        {
            if (my_rank != 0 || true) { //todo: remove "|| true"
                buffer2=  calcolaAvanzamentoContagi(buffer, buffer2, nazioneItem,
                        my_rank, t, d, people, storicoContatti,w,l);
            }

            buffer3 = mpi_gather(
                    buffer2,
                    buffer3,
                    -1,
                    (i + 1)
            );

            if (my_rank == 0 || true) //todo: remove "|| true"
            {
                System.out.println("End of the day:");
                printArray(buffer3);
            }
            else
            {
                buffer = buffer2;
            }
            buffer = buffer2; //todo: remove

            mpi_Barrier();
        }


    }

    private static void printArray(individualSummary[][] buffer3) {
        for (var i: buffer3)
        {
            for (var i2 : i){
                printInfoInfetti(i2);
            }
        }
    }

    private static void printInfoInfetti(individualSummary i2) {
        System.out.println(i2.rank + ": INFETTI " + i2.infetti + ", SANI " + i2.sani);
    }

    private static ArrayList<person> getPeople(nazione nazioneItem) {
        var people = new ArrayList<person>();
        for (subnazione i : nazioneItem.subnazioneArrayList) {
            people.addAll(i.people);
        }

        return people;
    }

    private static nazione GeneraMappa(int rank, int[] howManySubNationsPerProcess,
                                       int w, int l, individualSummary[][] start) {
        nazione n = new nazione();
        n.rank = rank;
        n.subnazioneArrayList = new ArrayList<>();
        for (int i=0; i<howManySubNationsPerProcess[rank]; i++)
        {
            subnazione r = new subnazione();
            r.p = new point();
            r.p.x = i % w;
            r.p.y = i/ l;
            r.rank = rank;
            r.nRettangoli = w*l;

            //r.rettangoli = GeneraRettangoli(w,l);
            r.people = new ArrayList<>();




            for (int k=0; k<start[rank][i].infetti; k++) {
                person personItem = new person();
                personItem.rank = rank;
                personItem.inInfected = true;
                personItem.p = new point();
                personItem.p.x = getRandomNumber(0, w);
                personItem.p.y = getRandomNumber(0, l);
                r.people.add(personItem);
            }

            for (int k=0; k<start[rank][i].sani; k++)
            {
                person personItem = new person();
                personItem.rank = rank;
                personItem.inInfected = false;
                personItem.p = new point();
                personItem.p.x = getRandomNumber(0, w);
                personItem.p.y = getRandomNumber(0, l);
                r.people.add(personItem);
            }

            n.subnazioneArrayList.add(r);
        }

        return n;
    }

    /*
    private static ArrayList<rettangolo> GeneraRettangoli(int w, int l) {
        ArrayList<rettangolo> r = new ArrayList<>();
        for (int i=0; i<w; i++)
        {
            for (int j=0; j<l; j++)
            {
                rettangolo rt = new rettangolo();
                rt.p = new point();
                rt.p.x = i;
                rt.p.y = j;
                r.add(rt);
            }
        }

        return r;

    }

     */



    private static individualSummary[][] mpi_scatter(individualSummary[][] start, individualSummary[][] buffer,
                                                     int howManyRectanglesPerProcess, int i) {
        //mpi
        buffer = start;
        return start;
    }

    private static void mpi_Barrier() {
        //mpi
    }

    private static individualSummary[][] mpi_gather(individualSummary[][] buffer2, individualSummary[][] buffer3,
                                                    int howManyRectanglesPerProcess, int i) {
        //mpi
        buffer3 = buffer2;
        return buffer2;
    }

    private static individualSummary[][] calcolaAvanzamentoContagi(
                individualSummary[][] buffer,
                individualSummary[][] buffer2, nazione nazioneItem,
                int rank, int t, int d,
                ArrayList<person> people,
                Dictionary<person, ArrayList<vicinanza>> storicoContatti,
                int w, int l)
    {
        buffer2 = buffer;

        individualSummary[] a = buffer[rank];

        System.out.println();

        int t2 = 60*60*24 / t;

        ArrayList<subnazione> subnazioneArrayList = GetSubnazioni(nazioneItem, rank);
        for (int sbi = 0; sbi< subnazioneArrayList.size(); sbi++) {
            var sb2 = subnazioneArrayList.get(sbi);



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

    private static ArrayList<subnazione> GetSubnazioni(nazione nazioneItem, int rank) {
        ArrayList<subnazione> r = new ArrayList<>();
        for (var i : nazioneItem.subnazioneArrayList)
        {
            if (i.rank == rank)
                r.add(i);
        }

        return r;
    }

    final static int minDurataPerInfettarsi = 10 * 60;
    final static int durataPerGuarire = 15 * 60;

    private static individualSummary[][]
        calcolaAvanzamentoContagi2(
            individualSummary[][] buffer,
            subnazione subNazioneItem, int rank,
            int t, int d, ArrayList<person> people,
            Dictionary<person, ArrayList<vicinanza>> storicoContatti,
            int w, int l, int i_t2, int subnazioneIndex)
    {


        if (buffer[rank][subnazioneIndex].infetti > 0) {

            for (person p : people) {



                if (t * i_t2 >= p.lastTimeHeWasInfected + durataPerGuarire && p.inInfected) {
                    p.inInfected = false;
                    buffer[rank][subnazioneIndex].infetti--;
                    buffer[rank][subnazioneIndex].sani++;
                }

                if (buffer[rank][subnazioneIndex].infetti > 0) {
                    ArrayList<Integer> r = trovaRettangolo(subNazioneItem, p, d, rank, w, l);
                    if (r != null) {
                        for (int i = 0, rSize = r.size(); i < rSize; i++) {
                            ArrayList<person> peopleNear = getPeopleNear(r.get(i), subNazioneItem, w, l);
                            for (person p2 : peopleNear) {
                                if (p2 != p && p2.inInfected) {
                                    var vicinanzaItem = TrovaVicinanza(p, p2, storicoContatti);
                                    if (vicinanzaItem == null) {
                                        vicinanza v = new vicinanza();
                                        v.from = p;
                                        v.timeFine = t * i_t2;
                                        v.timeInizio = t * i_t2;
                                        v.to = p2;
                                        var v2 = storicoContatti.get(p);
                                        if (v2 != null) {
                                            v2.add(v);
                                        } else {
                                            storicoContatti.put(p, new ArrayList<>());
                                            v2 = storicoContatti.get(p);
                                            v2.add(v);
                                        }

                                        if (v.timeFine - v.timeInizio >= minDurataPerInfettarsi && !p.inInfected) {
                                            p.inInfected = true;
                                            buffer[rank][subnazioneIndex].sani--;
                                            buffer[rank][subnazioneIndex].infetti++;
                                            p.lastTimeHeWasInfected = t * i_t2;
                                        }

                                    } else {
                                        vicinanzaItem.timeFine = t * i_t2;

                                        if (vicinanzaItem.timeFine - vicinanzaItem.timeInizio >= minDurataPerInfettarsi && !p.inInfected) {
                                            p.inInfected = true;
                                            buffer[rank][subnazioneIndex].sani--;
                                            buffer[rank][subnazioneIndex].infetti++;
                                            p.lastTimeHeWasInfected = t * i_t2;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        for (person p: people){

            int moveX = getRandomNumber(0,1);
            int moveY = getRandomNumber(0,1);

            if (moveX == 0)
                p.p.x ++;
            else
                p.p.x--;

            if (moveY == 0)
                p.p.y ++;
            else
                p.p.y--;

            if (p.p.x>=w)
            {
                p.p.x = w -1;
            }

            if (p.p.x < 0)
            {
                p.p.x = 0;
            }

            if (p.p.y>=l)
            {
                p.p.y = l -1;
            }

            if (p.p.y < 0)
            {
                p.p.y = 0;
            }
        }

        return buffer;
    }

    private static ArrayList<person> getPeopleNear(int rectIndex, subnazione subnazioneItem, int w, int l) {
        ArrayList<person> r = new ArrayList<>();

        point p = new point();
        p.x = rectIndex / w;
        p.y = rectIndex % l;

        for (var x : subnazioneItem.people){
            if (x.p.equals(p))
            {
                r.add(x);
            }
        }

        return r;
    }

    private static vicinanza TrovaVicinanza(person p, person p2, Dictionary<person,
            ArrayList<vicinanza>> storicoContatti) {

        var t1 = storicoContatti.get(p);
        if (t1 == null)
            return null;

        for (var t2: t1){
            if (t2.from == p && t2.to == p2)
                return t2;
        }


        return null;
    }

    private static ArrayList<Integer> trovaRettangolo(subnazione subnazioneItem,
                                                         person p, int d, int rank, int w, int l) {
        ArrayList<Integer> r = new ArrayList<>();
        for (int i=-d; i<d; i++)
        {
            for (int j=-d; j<d; j++)
            {
                int r3 = rectIndex(p.p.x + j, p.p.y + i, w,l);
                //rettangolo r2 = trovaRettangolo2(subnazioneItem, p.p.x + j, p.p.y + i, rank);
                if (r3 >=0 && r3<subnazioneItem.nRettangoli)
                {
                    r.add(r3);
                }
            }
        }

        return r;

    }

    public static int rectIndex(int x, int y, int w, int l){
        return (x * w) + y;
    }

    /*
    private static rettangolo trovaRettangolo2(subnazione subNazioneItem, int x, int y, int rank) {
        for (var i: subNazioneItem.rettangoli){
            if (i.p.x == x && i.p.y == y && subNazioneItem.rank == rank)
                return i;
        }

        return null;
    }


     */

    private static void printArray(individualSummary[][] buffer3, int rank) {

        for (individualSummary i2 : buffer3[rank]) {
            printInfoInfetti(i2);
        }

    }

    private static individualSummary[][] riempi(individualSummary[][] start, int numPeople, int numInfected,
                                                int numProcess, int[] maxRectanglesForEachProcess) {


        int sani = numPeople - numInfected;

        OptionalInt m1 = Arrays.stream(maxRectanglesForEachProcess).max();
        int maxRectanglesPerProcess= m1.getAsInt();

        for (int i=0; i< numProcess; i++) {
            start[i] = new individualSummary[maxRectanglesPerProcess];
            for (int j=0; j<maxRectanglesPerProcess; j++)
            {
                start[i][j] = new individualSummary();
                start[i][j].rank = (i* maxRectanglesPerProcess) + j;
            }
        }


        for (int i=0; i<numInfected; i++)
        {
            int where = -1;
            do {
                where = getRandomNumber(0, numProcess - 1);
            }
            while (maxRectanglesForEachProcess[where] -1 < 0);

            int where2 = getRandomNumber(0, maxRectanglesForEachProcess[where] -1 );
            start[where][where2].infetti++;
        }

        for (int i=0; i<sani; i++)
        {
            int where = -1;
            do {
                where = getRandomNumber(0, numProcess - 1);
            }
            while (maxRectanglesForEachProcess[where] -1 < 0);
            int where2 = getRandomNumber(0, maxRectanglesForEachProcess[where]-1);
            start[where][where2].sani++;
        }

        return start;

    }

    private static int[] calculateDistributionSubnationsToProcess(int numProcess, Integer numSubnations) {
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

        int[] r = new int[numProcess];

        int i=0;
        int done =0;
        while (done<numSubnations){

            r[i]++;

            i++;

            if (i>=numProcess)
            {
                i=0;
            }
            done++;
        }
        return r;
    }
}

class individualSummary{
    int sani;
    int infetti;
    int rank;
}



class vicinanza{
    int timeInizio;
    int timeFine;
    person from;
    person to;
}


class person {
    int rank;
    point p;
    boolean inInfected;
    int lastTimeHeWasInfected;
}

class nazione{
    ArrayList<subnazione> subnazioneArrayList;
    int rank;
}

class subnazione {
    ArrayList<person> people;
    //ArrayList<rettangolo> rettangoli;
    int nRettangoli;
    point p;
    int rank;
}
/*
class rettangolo
{
    point p;
}
*/


class point{
    int x;
    int y;

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        point point = (point) o;
        return x == point.x && y == point.y;
    }

    @Override
    public int hashCode() {
        return Objects.hash(x, y);
    }
}



