#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
//conversion en postfixe:
/*
Suivant l’article de Thompson, le compilateur construit un NFA à partir d’une expression régulière en notation postfix
infinx a(bb)+a To posfixe abb.+.a.
*/
char*reVersPostfix(char *re)
{
	int nalt, natom;
	static char buf[8000];
	char *dst;
	struct {
		int nalt;
		int natom;
	} paren[100], *p;
	
	p = paren;
	dst = buf;
	nalt = 0;
	natom = 0;
	if(strlen(re) >= sizeof buf/2)
		return NULL;
	for(; *re; re++){
		switch(*re){
		case '(':
			if(natom > 1){
				--natom;
				*dst++ = '.';
			}
			if(p >= paren+100)
				return NULL;
			p->nalt = nalt;
			p->natom = natom;
			p++;
			nalt = 0;
			natom = 0;
			break;
		case '|':
			if(natom == 0)
				return NULL;
			while(--natom > 0)
				*dst++ = '.';
			nalt++;
			break;
		case ')':
			if(p == paren)
				return NULL;
			if(natom == 0)
				return NULL;
			while(--natom > 0)
				*dst++ = '.';
			for(; nalt > 0; nalt--)
				*dst++ = '|';
			--p;
			nalt = p->nalt;
			natom = p->natom;
			natom++;
			break;
		case '*':
		case '+':
		case '?':
			if(natom == 0)
				return NULL;
			*dst++ = *re;
			break;
		default:
			if(natom > 1){
				--natom;
				*dst++ = '.';
			}
			*dst++ = *re;
			natom++;
			break;
		}
	}
	if(p != paren)
		return NULL;
	while(--natom > 0)
		*dst++ = '.';
	for(; nalt > 0; nalt--)
		*dst++ = '|';
	*dst = 0;
	return buf;
}



//Fin du regulier vers postfixe

//Definition de entiers qui caractérisant epsilon et état final 
enum
{
	epsilon = 256,
	final = 257
};

//Definition de structure qui represente les etats 
typedef struct Etat Etat;
//En basée sur un arber Binaire de Etat
struct Etat
{
	//etat actuel basé sur le code ascii
	int c;
	//pointeur vers état suivant
	Etat *out;
	//pointeurs utilise en cas de epsilon <=> en cas de "|"  avec stat *out
	Etat *out1;
	//la dernier List dans lequell s à partient utiliser ce champ pour la déterminisation
	int lastlist;
};

//Le nombre des états
int nEtat;

//Constructeur des états
Etat* Etate(int c, Etat *out, Etat *out1)
{
	Etat *s;
	
	nEtat++;
	s = (Etat*)malloc(sizeof (Etat));
	s->lastlist = 0;
	s->c = c;
	s->out = out;
	s->out1 = out1;
	return s;
}
//Fin du Constructeur 


//Fragment 
typedef struct Frag Frag;
//Ptrlist
typedef union Ptrlist Ptrlist;
//Couple(Etat,list de transition)
struct Frag
{
	Etat *start;
	//list chainée des etats
	Ptrlist *out;
};

//Constructeur de fragment
Frag frag(Etat *start, Ptrlist *out)
{
	Frag n = { start, out };
	return n;
}
//fin du constructeur

/*
Une union est un type de données spécial disponible en C qui permet de stocker différents types de données dans le même emplacement de mémoire.
Vous pouvez définir une union avec plusieurs membres, mais un seul membre peut contenir une valeur à un moment donné. 
Les unions offrent un moyen efficace d’utiliser le même emplacement de mémoire à des fins multiples.
*/
//list chainée
union Ptrlist
{
	Etat *s;
	//list des états suivant une etat s
	Ptrlist *next;
};

//constructeur de list qui prend comme argument une tableau de stats et ce dernier utilise pour construire une Fragment
//le resultat de cette fonction de fournie une  list chainée des Etats
Ptrlist*list1(Etat **outp)
{
	Ptrlist *l;
	l = (Ptrlist*)outp;
	l->next = NULL;
	return l;
}
//fin
//Connecte les flèches  de la liste l des pointeurs à l’état s
void patch(Ptrlist *l, Etat *s)
{
	Ptrlist *next;
	
	for(;l;l=next){
		next = l->next;
		l->s = s;
	}
}
//concatène deux listes de pointeurs
Ptrlist*append(Ptrlist *l1, Ptrlist *l2)
{
	Ptrlist *oldl1;
	oldl1 = l1;
	while(l1->next)
	l1 = l1->next;
	l1->next = l2;
	return oldl1;
}
//définition d'une état d'acceptation
 Etat acceptation = { final };
//:)
// Tansformateur Postfixe To NFA
Etat*postTONFA(char *postfix)
{
	char *p;
	Frag stack[1000], *stackp, e1, e2, e;
	Etat *s;
	

	if(postfix == NULL)
		return NULL;
	//Pointeur sur tableau <=> pile
	stackp = stack;
    // Pour la traitement du pile de fragment
	#define push(s) *stackp++ = s
	#define pop()   *--stackp

	for(p=postfix; *p; p++){
		switch(*p){
		case '.':
			e2 = pop();
			e1 = pop();
			//liée les tansitions de e1 vers Etat e2 
			patch(e1.out, e2.start);
			//saisir le resultat en pile
			push(frag(e1.start, e2.out));
			break;
		case '|':
			e2 = pop();
			e1 = pop();
			//Crée une nouvelle etats s et remlpie out et out1
			s = Etate(epsilon, e1.start, e2.start);
			//saisir le nouvelle couple s 
			push(frag(s,append(e1.out, e2.out)));
			break;
		case '*':
			e = pop();
			s = Etate(epsilon, e.start, NULL);
			patch(e.out, s);
			push(frag(s, list1(&s->out1)));
			break;
		case '+':
			e = pop();
			s = Etate(epsilon, e.start, NULL);
			patch(e.out, s);
			push(frag(e.start, list1(&s->out1)));
			break;
		default:
			s = Etate(*p, NULL, NULL);
			push(frag(s, list1(&s->out)));
			break;
		}
	}

	e = pop();
	if(stackp != stack)
		return NULL;
	
	patch(e.out, &acceptation);
	return e.start;
#undef pop
#undef push
}

/*-----------------------------------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------------------------------------
------------------------------------------------NFA-----------TO----------------DFA--------------------------------------------------
-------------------------------------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------------------------------*/
/*Tout NFA peut être converti en un DFA équivalent dans lequel chaque état DFA correspond à une liste d’états NFA.*/
/*Chaque état de la DFA correspond à une liste d’États de la NFA*/
//Pour calculer la clôture d'une étate on définie une structure List
typedef struct List List;
struct List
{
	//tableau des états
	Etat **s;
	//le nombre des élements
	int n;
};
List l1, l2;
//on utiliser un entier pour la distinction entre le différent nouveau etats 
static int listid;
//Structure de données qui représente DFA
typedef struct DFA DFA;
struct DFA
{
	//table qui contient l'ensemble des états
	List l;
	/*A DFA est la copie en cache de la liste l. Le tableau next contient des pointeurs vers l'état suivant pour chaque 
	caractère d'entrée possible : 
	si l'état actuel est d et que le caractère d'entrée suivant est c, alors d->next[c] est l'état suivant. Si d->next[c] est nul, alors
	l'état suivant n'a pas encore été calculé. NextEtat calcule, enregistre et renvoie l'état suivant pour un état et un caractère donné*/
	DFA *next[256];
	//les deux pointeurs sont utiliser pour organisé les DFAs en arber binaire de recherche pour facilité la recherche à les listes redandant
	DFA *left;
	DFA *right;
};

//Fonction qui permet d'ajouter une état à une List
void ajouterEtat(List *l, Etat *s)
{
	//la premier if est utilisé pour grantie s est insérer une seul fois
	if(s == NULL || s->lastlist == listid)
		return;
	s->lastlist = listid;
	if(s->c == epsilon){
		ajouterEtat(l, s->out);
		ajouterEtat(l, s->out1);
		return;
	}
	l->s[l->n++] = s;
}
//Fonction qui permet d'inialiser une liste à partir d'une état de départ
List* initList(Etat *s, List *l)
{
	listid++;
	l->n = 0;
	ajouterEtat(l, s);
	return l;
}
//Ce fonction permet de déterminer le nouveau List à partir d'une List et une caractère
void Transformation(List *ancienL, int c, List *nouveauL)
{
	int i;
	Etat *s;
    //on increment listid parce que on crée une nouvelle list
	listid++;
	nouveauL->n = 0;
	for(i=0; i<ancienL->n; i++){
		s = ancienL->s[i];
		if(s->c == c)
		//on ajoute les etats qui accepte une transformation après la lecture d'une caractére c
			ajouterEtat(nouveauL, s->out);
	}
}

/*Tous les DEtats qui ont été calculés doivent être sauvegardés dans une structure qui nous permet de rechercher a DEtat par son List.
Pour ce faire,nous les organisons dans un arbre binaire en utilisant le tri List comme clé. 
La dEtat fonction renvoie le DEtat pour un donné List, en en allouant un si nécessaire :*/
/*comparer deux List soit par leur taille ou par members*/
static int comparList(List *l1, List *l2)
{
	int i;

	if(l1->n < l2->n)
		return -1;
	if(l1->n > l2->n)
		return 1;
	for(i=0; i<l1->n; i++)
		if(l1->s[i] < l2->s[i])
			return -1;
		else if(l1->s[i] > l2->s[i])
			return 1;
	return 0;
}
/*Tous les DEtats qui ont été calculés doivent être sauvegardés dans une structure qui nous permet de rechercher a DEtat par son List. 
Pour ce faire, nous les organisons dans un arbre binaire en utilisant le tri List comme clé.
La dEtat fonction renvoie le DEtat pour un donné List, en en allouant un si nécessaire :*/
DFA *alldEtats;
DFA* dEtat(List *l)
{
	int i;
	DFA **dp, *d;
	/* regarder dans l'arborescence pour l'état existant */
	dp = &alldEtats;
	while((d = *dp) != NULL){
		i = comparList(l, &d->l);
		if(i < 0)
			dp = &d->left;
		else if(i > 0)
			dp = &d->right;
		else
		//s'il existe la liste le programme exit with return d.
			return d;
	}
	
	/* allouer, initialiser le nouvel état */
	d = malloc(sizeof *d + l->n*sizeof l->s[0]);
	memset(d, 0, sizeof *d);
	d->l.s = (Etat**)(d+1);
	memmove(d->l.s, l->s, l->n*sizeof l->s[0]);
	d->l.n = l->n;

	/* insert in tree */
	*dp = d;
	return d;
}

//initialié  DFA
DFA* init(Etat *start)
{
	return dEtat(initList(start, &l1));
}
//déterminer le résultat (nouveau DEtat) après la lecture d'une caractére c par DEtat ancienLne
DFA* nextEtat(DFA *d, int c)
{
	//l1 c'est une varaiable globale
	Transformation(&d->l, c, &l1);
	return d->next[c] = dEtat(&l1);
}

//Fin de transformation
//:)
int
connait(List *l)
{
	int i;

	for(i=0; i<l->n; i++)
		if(l->s[i] == &acceptation)
			return 1;
	return 0;
}
int testDereconna(DFA *start, char *s)
{
	DFA *d, *next;
	int c, i;
	
	d = start;
	for(; *s; s++){
		c = *s;
		if((next = d->next[c]) == NULL)
			next = nextEtat(d, c);
		d = next;
	}
	return connait(&d->l);
}

static int j=-1;
void AffichageDFA(DFA *d)
{
         j++;
         int i;
		for(i=0;i<d->l.n;i++){
        printf("%d(%d)\n",j,d->l.s[i]->c);
        }
	    if(d->left)  AffichageDFA(d->left);
        if(d->right) AffichageDFA(d->right);
	}

/*-------------------------------------------------------------------------------------------------------------------------------------------*/
//Application  de DFA
/*-------------------------------------------------------------------------------------------------------------------------------------------*/
int main()
{
		
	int i;
	char *post;
	Etat *start;
    char *s2="a(bb)+a";
    post=reVersPostfix(s2);
    printf("%s\n",post);
	start = postTONFA(post);
	l1.s = malloc(nEtat*sizeof l1.s[0]);
	l2.s = malloc(nEtat*sizeof l2.s[0]);
		if(testDereconna(init(start),"abba"))
		printf("%s\n","abba");
        DFA *d;
		DFA **dp;
		dp = &alldEtats;
		d=*dp;
        AffichageDFA(d);
	return 0;
}

