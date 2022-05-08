#include <stdio.h>
#include <stdlib.h>
#include<conio.h>
#include<string.h>
//Structure de données hybride arbre et pile
struct node {
    char data;
    struct node* left;
    struct node* right;
    struct node* next;
};
//On définie Racine
struct node *head=NULL;
//Constructeur des nodes
struct node* newNode(char data)
{
    struct node* node= (struct node*)malloc(sizeof(struct node));
    node->data = data;
    node->left = NULL;
    node->right = NULL;
    node->next = NULL;
    return (node);
}
//parcours d'arbre en ordre infixe
void printInorder(struct node* node)
{
    if (node == NULL)
        return;
    else{
     printInorder(node->left);
     printf("%c ", node->data);
     printInorder(node->right);
    }
}
 
void push(struct node* x)
{
    if(head==NULL)
    head = x;
    else
    {
        (x)->next = head;
        head  = x;
    }
   
}
struct node* pop()
{
    struct node* p = head;
    head = head->next;
    return p;
}
int main()
{   
char *s1;
s1=(char*)malloc(sizeof(char*));
printf("donner une expression regulier (l'expression doit se terminer par #):");
scanf("%s",s1);
int l=strlen(s1);
char s[l];
for (int i = 0; i < l;i++)
{
	    	s[i]=s1[i];

}
    struct node *x, *y, *z;
    for (int i = 0; i < l; i++) {
    	    

        if(s[i]!=')'&&s[i]!='('&&s[i]!='*'&&s[i]!='#'){
            y=newNode(s[i]);
        	push(y);
        }
      
		if(l==1){
			y=newNode(s[i]);
        	push(y);
        	break;
		}
        if(s[i]==')'){
        	z=pop();
        	y=pop();
        	x=pop();
        	y->left =x;
            y->right =z;
            push(y);
        }
		if(s[i]=='*'){
			y=newNode(s[i]);
			x=pop();
			y->left=x;
			push(y); 
		}
		if(s[i]=='#'){
		    z=pop();
        	y=pop();
        	x=pop();
        	y->left =x;
            y->right =z;
            push(y);
		}	
		}
    printf(" le parcours infinix d'arbre binaire est : ");
    printInorder(y);
    return 0;
}
