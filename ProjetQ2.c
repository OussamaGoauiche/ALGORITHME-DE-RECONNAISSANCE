#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
/*
1)On néglige les états parce que ne font pas partie du fonctionnement de la machine.
2)les transitions sont les clés de résolution de ce problème.
3)Le NFA d'une expression régulière est construit à partir de NFA partiels pour chaque sous-expression.
4)Les NFA partiels n'ont pas d'états correspondants : à la place, ils ont une ou plusieurs flèches pendantes, pointant vers rien. 
Le processus de construction se terminera en connectant ces flèches à un état correspondant.
*/
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

//Definition de entiers qui caractérisant division et état final 
enum
{
	division = 256,
	final = 257
};

//Structure qui represente les Transitions
typedef struct Etat Etat;
//En basée sur un arber Binaire de Etat
struct Etat
{
	//Caractère(Code ascii)
	int c;
	//pointeur vers état suivant
	Etat *suivant;
	//pointeurs utilise en cas de division <=> en cas de "|"  avec stat *suivant
	Etat *suivant1;
	//Utilise ce champ pour ajouter une etat à une liste
	int dernierlist;
};

//Parcours Le NFA
void  affiche(Etat *e){

		if (e)
		{
		printf("%c\n",e->c);
		if (e->suivant ) affiche(e->suivant);
		if (e->suivant1) affiche(e->suivant1);
		}
		else return;
	
}
//Le nombre des états
int netat;

//Constructeur des états
Etat* etat(int c, Etat *suivant, Etat *suivant1)
{
	Etat *s;
	
	netat++;
	s = (Etat*)malloc(sizeof (Etat));
	s->dernierlist = 0;
	s->c = c;
	s->suivant = suivant;
	s->suivant1 = suivant1;
	return s;
}
//Fin du Constructeur 


//FluxSortant
typedef struct FluxSortant FluxSortant;
//ptrverslist
typedef union ptrverslist ptrverslist;
//Couple(etat,Transition)
struct FluxSortant
{
	Etat *start;
	//list chainée des etats
	ptrverslist *suivant;
};

//Constructeur de FluxSortantment
FluxSortant fluxSortant(Etat *start, ptrverslist *suivant)
{
	FluxSortant n = { start, suivant };
	return n;
}
//fin du constructeur

/*
Une union est un type de données spécial disponible en C qui permet de stocker différents types de données dans le même emplacement de mémoire.
Vous pouvez définir une union avec plusieurs membres, mais un seul membre peut contenir une valeur à un moment donné. 
Les unions offrent un moyen efficace d’utiliser le même emplacement de mémoire à des fins multiples.
*/
//list chainée(regroupe l'états et ses transition en meme zone mémoire)
union ptrverslist
{
	Etat *s;
	//list des états suivant une etat s
	ptrverslist *next;
};

//constructeur de list qui prend comme argument une tableau d'états et ce dernier utilise pour construire une Flux Sortant
//le resultat de cette fonction de fournie une  list chainée des etats
ptrverslist*list1(Etat **suivantp)
{
	ptrverslist *l;
	l = (ptrverslist*)suivantp;
	l->next = NULL;
	return l;
}
//Connecte les flèches  de la liste l des pointeurs à l’état s
void liee(ptrverslist *l, Etat *s)
{
	ptrverslist *next;
	
	for(;l;l=next){
		next = l->next;
		l->s = s;
	}
}
//concatène deux listes de pointeurs
ptrverslist*concat(ptrverslist *l1, ptrverslist *l2)
{
	ptrverslist *oldl1;
	oldl1 = l1;
	while(l1->next)
	l1 = l1->next;
	l1->next = l2;
	return oldl1;
}
//:)
// Tansformateur Postfixe To NFA
Etat*postTONFA(char *postfix)
{
	char *p;
	FluxSortant pile[1000], *pilep, e1, e2, e;
	Etat *s;
	

	if(postfix == NULL)
		return NULL;
	//Pointeur sur tableau <=> pile
	pilep = pile;
    // Pour la traitement du pile de FluxSortantment
	#define push(s) *pilep++ = s
	#define pop()   *--pilep

	for(p=postfix; *p; p++){
		switch(*p){
		case '.':
			e2 = pop();
			e1 = pop();
			//liée les tansitions de e1 vers etat e2 
			liee(e1.suivant, e2.start);
			//saisir le resultat en pile
			push(fluxSortant(e1.start, e2.suivant));
			break;
		case '|':
			e2 = pop();
			e1 = pop();
			//Crée une nouvelle etats s et remlpie suivant et suivant1
			s = etat(division, e1.start, e2.start);
			//saisir le nouvelle couple s 
			push(fluxSortant(s,concat(e1.suivant, e2.suivant)));
			break;
		case '*':
			e = pop();
			s = etat(division, e.start, NULL);
			liee(e.suivant, s);
			push(fluxSortant(s, list1(&s->suivant1)));
			break;
		case '+':
			e = pop();
			s = etat(division, e.start, NULL);
			liee(e.suivant, s);
			push(fluxSortant(e.start, list1(&s->suivant1)));
			break;
		default:
			s = etat(*p, NULL, NULL);
			push(fluxSortant(s, list1(&s->suivant)));
			break;
		}
	}

	e = pop();
	if(pilep != pile)
		return NULL;
	
//définition d'une état d'acceptation
    Etat acceptation = { final };
	liee(e.suivant, &acceptation);
	affiche(e.start);
	return e.start;
#undef pop
#undef push
}







//Fin de trandformation
//:)

/*-------------------------------------------------------------------------------------------------------------------------------------------*/
//Application  de NFA
/*-------------------------------------------------------------------------------------------------------------------------------------------*/
int main()
{
	int i;
	char *postfixe;
	Etat *start;
    char *s2="a(bb)+a";
	postfixe = reVersPostfix(s2);
	printf("%s\n",postfixe);
	start = postTONFA(postfixe);
	//affiche(start);
	return 0;
}

