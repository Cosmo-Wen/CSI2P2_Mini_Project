#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/*
   For the language grammar, please refer to Grammar section on the github page:
https://github.com/lightbulb12294/CSI2P-II-Mini1#grammar
 */

#define MAX_REG 256

#define MAX_LENGTH 200

typedef enum {
	ASSIGN, ADD, SUB, MUL, DIV, REM, PREINC, PREDEC, POSTINC, POSTDEC, IDENTIFIER, CONSTANT, LPAR, RPAR, PLUS, MINUS, END
} Kind;

typedef enum {
	STMT, EXPR, ASSIGN_EXPR, ADD_EXPR, MUL_EXPR, UNARY_EXPR, POSTFIX_EXPR, PRI_EXPR
} GrammarState;

typedef struct TokenUnit {
	Kind kind;
	int val; // record the integer value or variable name
	struct TokenUnit *next;
} Token;

typedef struct ASTUnit {
	Kind kind;
	int val; // record the integer value or variable name
	struct ASTUnit *lhs, *mid, *rhs;
} AST;

/// utility interfaces

// err marco should be used when a expression error occurs.
#define err(x)\
{\
	puts("Compile Error!");\
	if(DEBUG) {\
		fprintf(stderr, "Error at line: %d\n", __LINE__);\
		fprintf(stderr, "Error message: %s\n", x);\
	}\
	exit(0);\
}

// You may set DEBUG=1 to debug. Remember setting back to 0 before submit.
#define DEBUG 1

// Split the input char array into token linked list.
Token *lexer(const char *in);

// Create a new token.
Token *new_token(Kind kind, int val);

// Translate a token linked list into array, return its length.
size_t token_list_to_arr(Token **head);

// Parse the token array. Return the constructed AST.
AST *parser(Token *arr, size_t len);

// Parse the token array. Return the constructed AST.
AST *parse(Token *arr, int l, int r, GrammarState S);

// Create a new AST node.
AST *new_AST(Kind kind, int val);

// Find the location of next token that fits the condition(cond). Return -1 if not found. Search direction from start to end.
int findNextSection(Token *arr, int start, int end, int (*cond)(Kind));

// Return 1 if kind is ASSIGN.
int condASSIGN(Kind kind);

// Return 1 if kind is ADD or SUB.
int condADD(Kind kind);

// Return 1 if kind is MUL, DIV, or REM.
int condMUL(Kind kind);

// Return 1 if kind is RPAR.
int condRPAR(Kind kind);

// Check if the AST is semantically right. This function will call err() automatically if check failed.
void semantic_check(AST *now);

// Generate ASM code.
// void codegen(AST *root);

///*
// Generate ASM code.
int codegen(AST *root);

// Find available register.
int newReg();

// Free register.
void freeReg(int reg);

void postIncDec();
//*/

// Free the whole AST.
void freeAST(AST *now);

/// debug interfaces

// Print token array.
void token_print(Token *in, size_t len);

// Print AST tree.
void AST_print(AST *head);

char input[MAX_LENGTH];

int reg_table[MAX_REG];

int var_alter[3];

int var_reg_ref[3] = {-1, -1, -1};

int main() 
{
	while (fgets(input, MAX_LENGTH, stdin) != NULL) {
		Token *content = lexer(input);
		size_t len = token_list_to_arr(&content);
		token_print(content, len);
		if (len == 0) 
			continue;
		AST *ast_root = parser(content, len);
		AST_print(ast_root);
		semantic_check(ast_root);
		codegen(ast_root);
		postIncDec();
		free(content);
		freeAST(ast_root);
	}
	
	return 0;
}

// Split the input char array into token linked list.
Token *lexer(const char *in) 
{
	Token *head = NULL;
	Token **now = &head;
	
	for (int i = 0; in[i]; i++) {
		if (isspace(in[i])) // ignore space characters
			continue;
		else if (isdigit(in[i])) {
			(*now) = new_token(CONSTANT, atoi(in + i));
			while (in[i+1] && isdigit(in[i+1])) i++;
		}
		else if ('x' <= in[i] && in[i] <= 'z') // variable
			(*now) = new_token(IDENTIFIER, in[i]);
		else switch (in[i]) {
			case '=':
				(*now) = new_token(ASSIGN, 0);
				break;
			case '+':
				if (in[i+1] && in[i+1] == '+') {
					i++;
					// In lexer scope, all "++" will be labeled as PREINC.
					(*now) = new_token(PREINC, 0);
				}
				// In lexer scope, all single "+" will be labeled as PLUS.
				else (*now) = new_token(PLUS, 0);
				break;
			case '-':
				if (in[i+1] && in[i+1] == '-') {
					i++;
					// In lexer scope, all "--" will be labeled as PREDEC.
					(*now) = new_token(PREDEC, 0);
				}
				// In lexer scope, all single "-" will be labeled as MINUS.
				else (*now) = new_token(MINUS, 0);
				break;
			case '*':
				(*now) = new_token(MUL, 0);
				break;
			case '/':
				(*now) = new_token(DIV, 0);
				break;
			case '%':
				(*now) = new_token(REM, 0);
				break;
			case '(':
				(*now) = new_token(LPAR, 0);
				break;
			case ')':
				(*now) = new_token(RPAR, 0);
				break;
			case ';':
				(*now) = new_token(END, 0);
				break;
			default:
				err("Unexpected character.");
		}
		now = &((*now)->next);
	}
	return head;
}

// Create a new token.
Token *new_token(Kind kind, int val) 
{
	Token *res = (Token*)malloc(sizeof(Token));
	
	res->kind = kind;
	res->val = val;
	res->next = NULL;
	return res;
}

// Translate a token linked list into array, return its length.
size_t token_list_to_arr(Token **head) 
{
	size_t res;
	Token *now = (*head), *del;
	
	for (res = 0; now != NULL; res++) now = now->next;
	now = (*head);
	if (res != 0) (*head) = (Token*)malloc(sizeof(Token) * res);
	for (int i = 0; i < res; i++) {
		(*head)[i] = (*now);
		del = now;
		now = now->next;
		free(del);
	}
	return res;
}

// Parse the token array. Return the constructed AST.
AST *parser(Token *arr, size_t len) 
{
	for (int i = 1; i < len; i++) {
		// correctly identify "ADD" and "SUB"
		if (arr[i].kind == PLUS || arr[i].kind == MINUS) {
			switch (arr[i - 1].kind) {
				case PREINC:
				case PREDEC:
				case IDENTIFIER:
				case CONSTANT:
				case RPAR:
					arr[i].kind = arr[i].kind - PLUS + ADD;
				default: break;
			}
		}
	}
	return parse(arr, 0, len - 1, STMT);
}

// Parse the token array. Return the constructed AST.
AST *parse(Token *arr, int l, int r, GrammarState S) 
{
	AST *now = NULL;
	
	if (l > r)
		err("Unexpected parsing range.");
	int nxt;
	switch (S) {
		case STMT:
			if (l == r && arr[l].kind == END)
				return NULL;
			else if (arr[r].kind == END)
				return parse(arr, l, r - 1, EXPR);
			else err("Expected \';\' at the end of line.");
		case EXPR:
			return parse(arr, l, r, ASSIGN_EXPR);
		case ASSIGN_EXPR:
			if ((nxt = findNextSection(arr, l, r, condASSIGN)) != -1) {
				now = new_AST(arr[nxt].kind, 0);
				now->lhs = parse(arr, l, nxt - 1, UNARY_EXPR);
				now->rhs = parse(arr, nxt + 1, r, ASSIGN_EXPR);
				return now;
			}
			return parse(arr, l, r, ADD_EXPR);
		case ADD_EXPR:
			if((nxt = findNextSection(arr, r, l, condADD)) != -1) {
				now = new_AST(arr[nxt].kind, 0);
				now->lhs = parse(arr, l, nxt - 1, ADD_EXPR);
				now->rhs = parse(arr, nxt + 1, r, MUL_EXPR);
				return now;
			}
			return parse(arr, l, r, MUL_EXPR);
		case MUL_EXPR:
			// TODO: Implement MUL_EXPR.
			// hint: Take ADD_EXPR as reference.
			if ((nxt = findNextSection(arr, r, l, condMUL)) != -1) {
				now = new_AST(arr[nxt].kind, 0);
				now->lhs = parse(arr, l, nxt - 1, MUL_EXPR);
				now->rhs = parse(arr, nxt + 1, r, UNARY_EXPR);
				return now;
			}
			return parse(arr, l, r, UNARY_EXPR);
		case UNARY_EXPR:
			// TODO: Implement UNARY_EXPR.
			// hint: Take POSTFIX_EXPR as reference.
			if (arr[l].kind == PREINC || arr[l].kind == PREDEC || arr[l].kind == PLUS || arr[l].kind == MINUS) {
				now = new_AST(arr[l].kind, 0);
				now->mid = parse(arr, l + 1, r, UNARY_EXPR);
				return now;
			}
			return parse(arr, l, r, POSTFIX_EXPR);
		case POSTFIX_EXPR:
			if (arr[r].kind == PREINC || arr[r].kind == PREDEC) {
				// translate "PREINC", "PREDEC" into "POSTINC", "POSTDEC"
				now = new_AST(arr[r].kind - PREINC + POSTINC, 0);
				now->mid = parse(arr, l, r - 1, POSTFIX_EXPR);
				return now;
			}
			return parse(arr, l, r, PRI_EXPR);
		case PRI_EXPR:
			if (findNextSection(arr, l, r, condRPAR) == r) {
				now = new_AST(LPAR, 0);
				now->mid = parse(arr, l + 1, r - 1, EXPR);
				return now;
			}
			if (l == r) {
				if (arr[l].kind == IDENTIFIER || arr[l].kind == CONSTANT)
					return new_AST(arr[l].kind, arr[l].val);
				err("Unexpected token during parsing.");
			}
			err("No token left for parsing.");
		default:
			err("Unexpected grammar state.");
	}
}

// Create a new AST node.
AST *new_AST(Kind kind, int val) 
{
	AST *res = (AST*)malloc(sizeof(AST));
	res->kind = kind;
	res->val = val;
	res->lhs = res->mid = res->rhs = NULL;
	return res;
}

// Find the location of next token that fits the condition(cond). Return -1 if not found. Search direction from start to end.
int findNextSection(Token *arr, int start, int end, int (*cond)(Kind)) 
{
	int par = 0;
	int d = (start < end) ? 1: -1;
	
	for (int i = start; (start < end) ? (i <= end): (i >= end); i += d) {
		if (arr[i].kind == LPAR) par++;
		if (arr[i].kind == RPAR) par--;
		if (par == 0 && cond(arr[i].kind) == 1) return i;
	}
	return -1;
}

// Return 1 if kind is ASSIGN.
int condASSIGN(Kind kind) 
{
	return kind == ASSIGN;
}

// Return 1 if kind is ADD or SUB.
int condADD(Kind kind) 
{
	return kind == ADD || kind == SUB;
}

// Return 1 if kind is MUL, DIV, or REM.
int condMUL(Kind kind) 
{
	return kind == MUL || kind == DIV || kind == REM;
}

// Return 1 if kind is RPAR.
int condRPAR(Kind kind) 
{
	return kind == RPAR;
}

// Check if the AST is semantically right. This function will call err() automatically if check failed.
void semantic_check(AST *now) 
{
	AST *tmp;

	if (now == NULL) 
		return;
	// Left operand of '=' must be an identifier or identifier with one or more parentheses.
	if (now->kind == ASSIGN) {
		tmp = now->lhs;
		while (tmp->kind == LPAR) 
			tmp = tmp->mid;
		if (tmp->kind != IDENTIFIER) {
			err("Lvalue is required as left operand of assignment.");
			return ;
		}
		semantic_check(now->rhs);
	}
	// Operand of INC/DEC must be an identifier or identifier with one or more parentheses.
	// TODO: Implement the remaining semantic_check code.
	// hint: Follow the instruction above and ASSIGN-part code to implement.
	// hint: Semantic of each node needs to be checked recursively (from the current node to lhs/mid/rhs node).
	else if (now->kind == PREINC ||	now->kind == PREDEC ||	now->kind == POSTINC || now->kind == POSTDEC) {
		tmp = now->mid;
		while (tmp->kind == LPAR)
			tmp = tmp->mid;
		if (tmp->kind != IDENTIFIER) {
			if (now->kind == PREINC || now->kind == PREDEC) {
				err("Rvalue is required as right operand of prefix operation.");
			}
			else if (now->kind == POSTINC || now->kind == POSTDEC) {
				err("Lvalue is required as left operand of postfix operation");
			}
		}
		return ;
	}
	else {
		semantic_check(now->lhs);
		semantic_check(now->mid);
		semantic_check(now->rhs);
	}

	return ;
}

// Generate ASM code.
// void codegen(AST *root) {}

///*
int codegen(AST *root)
{
	AST *tmp;
	int reg, reg1, reg2;
	int i;

	// TODO: Implement your codegen in your own way.
	// You may modify the function parameter or the return type, even the whole structure as you wish.
	if (root == NULL)
		return -1;
	switch (root->kind) {
		case ASSIGN:
			tmp = root->lhs;
			while (tmp->kind == LPAR)
				tmp = tmp->mid;
			reg = codegen(root->rhs);
			if (tmp->val == 'x') {
				printf("store [0] r%d\n", reg);
				var_reg_ref[0] = reg;
			}
			else if (tmp->val == 'y') {
				printf("store [4] r%d\n", reg);
				var_reg_ref[1] = reg;
			}
			else {
				printf("store [8] r%d\n", reg);
				var_reg_ref[2] = reg;
			}
			return reg;

		case ADD:
			if (root->lhs->kind != CONSTANT && root->rhs->kind != CONSTANT) {
				reg1 = codegen(root->lhs);
				reg2 = codegen(root->rhs);
				reg = newReg();
				printf("add r%d r%d r%d\n", reg, reg1, reg2);
				for (i = 0; i < 3; i++) {
					if (reg1 == var_reg_ref[i])
						break;
				}
				if (i >= 3)
					freeReg(reg1);

				for (i = 0; i < 3; i++) {
					if (reg2 == var_reg_ref[i])
						break;
				}
				if (i >= 3)
					freeReg(reg2);;
			}
			else if (root->lhs->kind == CONSTANT && root->rhs->kind != CONSTANT) {
				reg2 = codegen(root->rhs);
				reg = newReg();
				printf("add r%d %d r%d\n", reg, root->lhs->val, reg2);
				for (i = 0; i < 3; i++) {
					if (reg2 == var_reg_ref[i])
						break;
				}
				if (i >= 3)
					freeReg(reg2);;
			}
			else if (root->lhs->kind != CONSTANT && root->rhs->kind == CONSTANT) {
				reg1 = codegen(root->lhs);
				reg = newReg();
				printf("add r%d r%d %d\n", reg, reg1, root->rhs->val);
				for (i = 0; i < 3; i++) {
					if (reg1 == var_reg_ref[i])
						break;
				}
				if (i >= 3)
					freeReg(reg1);;
			}
			else {
				reg = newReg();
				printf("add r%d %d %d\n", reg, root->lhs->val, root->rhs->val);
			}
			return reg;

		case SUB:
			if (root->lhs->kind != CONSTANT && root->rhs->kind != CONSTANT) {
				reg1 = codegen(root->lhs);
				reg2 = codegen(root->rhs);
				reg = newReg();
				printf("sub r%d r%d r%d\n", reg, reg1, reg2);
				for (i = 0; i < 3; i++) {
					if (reg1 == var_reg_ref[i])
						break;
				}
				if (i >= 3)
					freeReg(reg1);;
				for (i = 0; i < 3; i++) {
					if (reg2 == var_reg_ref[i])
						break;
				}
				if (i >= 3)
					freeReg(reg2);;
			}
			else if (root->lhs->kind == CONSTANT && root->rhs->kind != CONSTANT) {
				reg2 = codegen(root->rhs);
				reg = newReg();
				printf("sub r%d %d r%d\n", reg, root->lhs->val, reg2);
				for (i = 0; i < 3; i++) {
					if (reg2 == var_reg_ref[i])
						break;
				}
				if (i >= 3)
					freeReg(reg2);;
			}
			else if (root->lhs->kind != CONSTANT && root->rhs->kind == CONSTANT) {
				reg1 = codegen(root->lhs);
				reg = newReg();
				printf("sub r%d r%d %d\n", reg, reg1, root->rhs->val);
				for (i = 0; i < 3; i++) {
					if (reg1 == var_reg_ref[i])
						break;
				}
				if (i >= 3)
					freeReg(reg1);;
			}
			else {
				reg = newReg();
				printf("sub r%d %d %d\n", reg, root->lhs->val, root->rhs->val);
			}
			return reg;

		case MUL:
			if (root->lhs->kind != CONSTANT && root->rhs->kind != CONSTANT) {
				reg1 = codegen(root->lhs);
				reg2 = codegen(root->rhs);
				reg = newReg();
				printf("mul r%d r%d r%d\n", reg, reg1, reg2);
				for (i = 0; i < 3; i++) {
					if (reg1 == var_reg_ref[i])
						break;
				}
				if (i >= 3)
					freeReg(reg1);;
				for (i = 0; i < 3; i++) {
					if (reg2 == var_reg_ref[i])
						break;
				}
				if (i >= 3)
					freeReg(reg2);;
			}
			else if (root->lhs->kind == CONSTANT && root->rhs->kind != CONSTANT) {
				reg2 = codegen(root->rhs);
				reg = newReg();
				printf("mul r%d %d r%d\n", reg, root->lhs->val, reg2);
				for (i = 0; i < 3; i++) {
					if (reg2 == var_reg_ref[i])
						break;
				}
				if (i >= 3)
					freeReg(reg2);;
			}
			else if (root->lhs->kind != CONSTANT && root->rhs->kind == CONSTANT) {
				reg1 = codegen(root->lhs);
				reg = newReg();
				printf("mul r%d r%d %d\n", reg, reg1, root->rhs->val);
				for (i = 0; i < 3; i++) {
					if (reg1 == var_reg_ref[i])
						break;
				}
				if (i >= 3)
					freeReg(reg1);;
			}
			else {
				reg = newReg();
				printf("mul r%d %d %d\n", reg, root->lhs->val, root->rhs->val);
			}
			return reg;

		case DIV:
			if (root->lhs->kind != CONSTANT && root->rhs->kind != CONSTANT) {
				reg1 = codegen(root->lhs);
				reg2 = codegen(root->rhs);
				reg = newReg();
				printf("div r%d r%d r%d\n", reg, reg1, reg2);
				for (i = 0; i < 3; i++) {
					if (reg1 == var_reg_ref[i])
						break;
				}
				if (i >= 3)
					freeReg(reg1);;
				for (i = 0; i < 3; i++) {
					if (reg2 == var_reg_ref[i])
						break;
				}
				if (i >= 3)
					freeReg(reg2);;
			}
			else if (root->lhs->kind == CONSTANT && root->rhs->kind != CONSTANT) {
				reg2 = codegen(root->rhs);
				reg = newReg();
				printf("div r%d %d r%d\n", reg, root->lhs->val, reg2);
				for (i = 0; i < 3; i++) {
					if (reg2 == var_reg_ref[i])
						break;
				}
				if (i >= 3)
					freeReg(reg2);;
			}
			else if (root->lhs->kind != CONSTANT && root->rhs->kind == CONSTANT) {
				reg1 = codegen(root->lhs);
				reg = newReg();
				printf("div r%d r%d %d\n", reg, reg1, root->rhs->val);
				for (i = 0; i < 3; i++) {
					if (reg1 == var_reg_ref[i])
						break;
				}
				if (i >= 3)
					freeReg(reg1);;
			}
			else {
				reg = newReg();
				printf("div r%d %d %d\n", reg, root->lhs->val, root->rhs->val);
			}
			return reg;

		case REM:
			if (root->lhs->kind != CONSTANT && root->rhs->kind != CONSTANT) {
				reg1 = codegen(root->lhs);
				reg2 = codegen(root->rhs);
				reg = newReg();
				printf("rem r%d r%d r%d\n", reg, reg1, reg2);
				for (i = 0; i < 3; i++) {
					if (reg1 == var_reg_ref[i])
						break;
				}
				if (i >= 3)
					freeReg(reg1);;
				for (i = 0; i < 3; i++) {
					if (reg2 == var_reg_ref[i])
						break;
				}
				if (i >= 3)
					freeReg(reg2);;
			}
			else if (root->lhs->kind == CONSTANT && root->rhs->kind != CONSTANT) {
				reg2 = codegen(root->rhs);
				reg = newReg();
				printf("rem r%d %d r%d\n", reg, root->lhs->val, reg2);
				for (i = 0; i < 3; i++) {
					if (reg2 == var_reg_ref[i])
						break;
				}
				if (i >= 3)
					freeReg(reg2);;
			}
			else if (root->lhs->kind != CONSTANT && root->rhs->kind == CONSTANT) {
				reg1 = codegen(root->lhs);
				reg = newReg();
				printf("rem r%d r%d %d\n", reg, reg1, root->rhs->val);
				for (i = 0; i < 3; i++) {
					if (reg1 == var_reg_ref[i])
						break;
				}
				if (i >= 3)
					freeReg(reg1);;
			}
			else {
				reg = newReg();
				printf("rem r%d %d %d\n", reg, root->lhs->val, root->rhs->val);
			}
			return reg;

		case PREINC:
			reg = codegen(root->mid);
			printf("add r%d r%d 1\n", reg, reg);
			for (i = 0; i < 3; i++) {
				if (reg == var_reg_ref[i]) {
					printf("store [%d] r%d\n", 4 * i, reg);
					break;
				}
			}
			if (i >= 3) {
				tmp = root->mid;
				while (tmp->kind == LPAR)
					tmp = tmp->mid;
				if (tmp->val == 'x')
					i = 0;
				else if (tmp->val == 'y')
					i = 4;
				else
					i = 8;
				printf("store [%d] r%d\n", i, reg);
			}
			return reg;

		case PREDEC:
			reg = codegen(root->mid);
			printf("sub r%d r%d 1\n", reg, reg);
			for (i = 0; i < 3; i++) {
				if (reg == var_reg_ref[i]) {
					printf("store [%d] r%d\n", 4 * i, reg);
					break;
				}
			}
			if (i >= 3) {
				tmp = root->mid;
				while (tmp->kind == LPAR)
					tmp = tmp->mid;
				if (tmp->val == 'x')
					i = 0;
				else if (tmp->val == 'y')
					i = 4;
				else
					i = 8;
				printf("store [%d] r%d\n", i, reg);
			}
			return reg;

		case POSTINC:
			tmp = root->mid;
			while (tmp->kind == LPAR)
				tmp = tmp->mid;
			if (tmp->val == 'x')
				var_alter[0]++;
			else if (tmp->val == 'y')
				var_alter[1]++;
			else 
				var_alter[2]++;
			return codegen(tmp);

		case POSTDEC:
			tmp = root->mid;
			while (tmp->kind == LPAR)
				tmp = tmp->mid;
			if (tmp->val == 'x')
				var_alter[0]--;
			else if (tmp->val == 'y')
				var_alter[1]--;
			else 
				var_alter[2]--;
			return codegen(tmp);

		case IDENTIFIER:
			if (root->val == 'x') {
				if (var_reg_ref[0] < 0) {
					reg = newReg();
					printf("load r%d [0]\n", reg);
					var_reg_ref[0] = reg;
				}
				else
					reg = var_reg_ref[0];
			}
			else if (root->val == 'y') {
				if (var_reg_ref[1] < 0) {
					reg = newReg();
					printf("load r%d [4]\n", reg);
					var_reg_ref[1] = reg;
				}
				else
					reg = var_reg_ref[1];
			}
			else {
				if (var_reg_ref[2] < 0) {
					reg = newReg();
					printf("load r%d [8]\n", reg);
					var_reg_ref[2] = reg;
				}
				else
					reg = var_reg_ref[2];
			}
			return reg;

		case CONSTANT:
			reg = newReg();
			printf("add r%d r%d %d\n", reg, reg, root->val);
			return reg;

		case LPAR:
			tmp = root;
			while (tmp->kind == LPAR)
				tmp = tmp->mid;
			reg = codegen(tmp);
			return reg;

		case PLUS:
			return codegen(root->mid);

		case MINUS:
			reg2 = codegen(root->mid);
			reg1 = newReg();
			printf("sub r%d 0 r%d\n", reg1, reg2);
			return reg1;

		default: ;
	}
	err("Invalid AST node in tree.");

	return -1;
}

int newReg()
{
	int i;

	for (i = 0; i < MAX_REG; i++) {
		if (!reg_table[i]) {
			reg_table[i] = 1;
			return i;
		}
	}

	err("Register depleted.");
}

void freeReg(int reg)
{
	reg_table[reg] = 0;
	printf("sub r%d r%d r%d\n", reg, reg, reg);

	return ;
}

void postIncDec()
{
	int reg;
	int i;

	for (i = 0; i < 3; i++) {
		if (var_alter[i] != 0) {
			if (var_reg_ref[i] < 0) {
				reg = newReg();
				printf("load r%d [%d]\n", reg, i * 4);
			}
			else
				reg = var_reg_ref[i];
			if (var_alter[i] > 0)
				printf("add r%d r%d %d\n", reg, reg, var_alter[i]);
			else
				printf("sub r%d r%d %d\n", reg, reg, var_alter[i] * -1);
			printf("store [%d] r%d\n", i * 4, reg);
			var_alter[i] = 0;
		}
	}

	return ;
}
//*/

// Free the whole AST.
void freeAST(AST *now) 
{
	if (now == NULL) return;
	freeAST(now->lhs);
	freeAST(now->mid);
	freeAST(now->rhs);
	free(now);
}

// Print token array.
void token_print(Token *in, size_t len) 
{
	const static char KindName[][20] = {
		"Assign", "Add", "Sub", "Mul", "Div", "Rem", "Inc", "Dec", "Inc", "Dec", "Identifier", "Constant", "LPar", "RPar", "Plus", "Minus", "End"
	};
	const static char KindSymbol[][20] = {
		"'='", "'+'", "'-'", "'*'", "'/'", "'%'", "\"++\"", "\"--\"", "\"++\"", "\"--\"", "", "", "'('", "')'", "'+'", "'-'"
	};
	const static char format_str[] = "<Index = %3d>: %-10s, %-6s = %s\n";
	const static char format_int[] = "<Index = %3d>: %-10s, %-6s = %d\n";

	for(int i = 0; i < len; i++) {
		switch(in[i].kind) {
			case LPAR:
			case RPAR:
			case PREINC:
			case PREDEC:
			case ADD:
			case SUB:
			case MUL:
			case DIV:
			case REM:
			case ASSIGN:
			case PLUS:
			case MINUS:
				printf(format_str, i, KindName[in[i].kind], "symbol", KindSymbol[in[i].kind]);
				break;
			case CONSTANT:
				printf(format_int, i, KindName[in[i].kind], "value", in[i].val);
				break;
			case IDENTIFIER:
				printf(format_str, i, KindName[in[i].kind], "name", (char*)(&(in[i].val)));
				break;
			case END:
				printf("<Index = %3d>: %-10s\n", i, KindName[in[i].kind]);
				break;
			default:
				puts("=== unknown token ===");
		}
	}
}

// Print AST tree.
void AST_print(AST *head) 
{
	static char indent_str[MAX_LENGTH] = "";
	static int indent = 0;
	char *indent_now = indent_str + indent;
	const static char KindName[][20] = {
		"Assign", "Add", "Sub", "Mul", "Div", "Rem", "PreInc", "PreDec", "PostInc", "PostDec", 
		"Identifier", "Constant", "Parentheses", "Parentheses", "Plus", "Minus"
	};
	const static char format[] = "%s\n";
	const static char format_str[] = "%s, <%s = %s>\n";
	const static char format_val[] = "%s, <%s = %d>\n";

	if (head == NULL) return;
	indent_str[indent - 1] = '-';
	printf("%s", indent_str);
	indent_str[indent - 1] = ' ';
	if (indent_str[indent - 2] == '`')
		indent_str[indent - 2] = ' ';
	switch (head->kind) {
		case ASSIGN:
		case ADD:
		case SUB:
		case MUL:
		case DIV:
		case REM:
		case PREINC:
		case PREDEC:
		case POSTINC:
		case POSTDEC:
		case LPAR:
		case RPAR:
		case PLUS:
		case MINUS:
			printf(format, KindName[head->kind]);
			break;
		case IDENTIFIER:
			printf(format_str, KindName[head->kind], "name", (char*)&(head->val));
			break;
		case CONSTANT:
			printf(format_val, KindName[head->kind], "value", head->val);
			break;
		default:
			puts("=== unknown AST type ===");
	}
	indent += 2;
	strcpy(indent_now, "| ");
	AST_print(head->lhs);
	strcpy(indent_now, "` ");
	AST_print(head->mid);
	AST_print(head->rhs);
	indent -= 2;
	(*indent_now) = '\0';
}
