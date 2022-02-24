// SPDX-License-Identifier: GPL-2.0-or-later

#include <assert.h>
#include <inttypes.h>
#include <savl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct employee {
	char			*family_name;
	char			*given_name;
	struct savl_node	name_node;
	struct savl_node	number_node;
	uint32_t		employee_number;
};

/*
 * Macros can handle both const and non-const pointers
 */

#define EMPLOYEE_FROM_NAME_NODE(n)	\
	((n) ? SAVL_NODE_CONTAINER((n), struct employee, name_node) : NULL)

#define EMPLOYEE_FROM_NUMBER_NODE(n)	\
	((n) ? SAVL_NODE_CONTAINER((n), struct employee, number_node) : NULL)

static struct savl_node *employees_by_name;
static struct savl_node *employees_by_number;

static int cmp_names(const union savl_key k, const struct savl_node *const n)
{
	const struct employee *key, *node;
	int result;

	key = k.p;
	node = EMPLOYEE_FROM_NAME_NODE(n);

	result = strcmp(key->family_name, node->family_name);
	if (result != 0)
		return result;

	return strcmp(key->given_name, node->given_name);
}

static int cmp_numbers(const union savl_key k, const struct savl_node *const n)
{
	struct employee *node;
	uint32_t key;

	key = k.u;
	node = EMPLOYEE_FROM_NUMBER_NODE(n);

	/* Subtraction might underflow */
	return (key > node->employee_number) - (key < node->employee_number);
}

static void free_by_name(struct savl_node *const n)
{
	struct employee *emp;

	emp = EMPLOYEE_FROM_NAME_NODE(n);
	free(emp->given_name);
	free(emp->family_name);
	free(emp);
}

static _Bool add_employee(struct employee *emp)
{
	struct savl_node *existing;
	union savl_key key;

	/* Check for an existing employee with the same number */
	key.u = emp->employee_number;
	existing = savl_get(employees_by_number, cmp_numbers, key);
	if (existing != NULL) {
		fprintf(stderr, "Employee #%" PRIu32 " already exists!\n",
			emp->employee_number);
		return 0;
	}

	/* Try to add the employee to the by-name tree */
	key.p = emp;
	existing = savl_try_add(&employees_by_name, cmp_names, key,
				&emp->name_node);
	if (existing != NULL) {
		fprintf(stderr, "Employee %s, %s already exists!\n",
			emp->family_name, emp->given_name);
		return 0;
	}

	/* Actually add the employee to the by-number tree */
	key.u = emp->employee_number;
	existing = savl_try_add(&employees_by_number, cmp_numbers, key,
				&emp->number_node);
	assert(existing == NULL);

	return 1;
}

static struct employee *get_by_name(char *restrict const family_name,
				    char *restrict const given_name)
{
	/* cmp_names() wants an employee struct as its key */
	struct employee emp;
	union savl_key key;
	struct savl_node *node;

	emp.family_name = family_name;
	emp.given_name = given_name;
	key.p = &emp;

	node = savl_get(employees_by_name, cmp_names, key);

	return EMPLOYEE_FROM_NAME_NODE(node);
}

static struct employee *get_by_number(const uint32_t employee_number)
{
	union savl_key key;
	struct savl_node *node;

	key.u = employee_number;

	node = savl_get(employees_by_number, cmp_numbers, key);

	return EMPLOYEE_FROM_NUMBER_NODE(node);
}

static char *checked_strdup(const char *const s)
{
	char *new;

	new = strdup(s);

	if (new == NULL) {
		fputs("Memory allocation failure\n", stderr);
		abort();
	}

	return new;
}

/* Some test data */
static const struct {
	const char	*family_name;
	const char	*given_name;
	uint32_t	employee_number;
}
employees[] = {
	{ "Oldrich", "Sharif", 5403298 },
	{ "Uno", "Eleri", 498302 },
	{ "Lykos", "Paavali", 4890 },
	{ "Villum", "Irmina", 498302 },  /* DUPLICATE EMPLOYEE NUMBER */
	{ "Feivush", "Georg", 49803 },
	{ "Zumra", "Kehina", 4123 },
	{ "Feivush", "Georg", 98021 },  /* DUPLICATE NAME */
	{ "Mahmut", "Sif", 509 },
	{ "Chidimma", "Pankaj", 874189 }
};

int main(void)
{
	struct employee *emp;
	struct savl_node *n;
	unsigned int i;

	/* Load the data */
	for (i = 0; i < sizeof employees / sizeof employees[0]; ++i) {

		emp = calloc(1, sizeof *emp);
		if (emp == NULL) {
			fputs("Memory allocation failure\n", stderr);
			abort();
		}

		emp->family_name = checked_strdup(employees[i].family_name);
		emp->given_name = checked_strdup(employees[i].given_name);
		emp->employee_number = employees[i].employee_number;

		if (!add_employee(emp))
			free_by_name(&emp->name_node);
	}

	puts("\nList of employees by name:");

	for (n = savl_first(employees_by_name); n != NULL; n = savl_next(n)) {

		emp = EMPLOYEE_FROM_NAME_NODE(n);
		printf("  %s, %s: %" PRIu32 "\n", emp->family_name,
		       emp->given_name, emp->employee_number);
	}

	puts("\nList of employees by number:");

	for (n = savl_first(employees_by_number); n != NULL; n = savl_next(n)) {

		emp = EMPLOYEE_FROM_NUMBER_NODE(n);
		printf("  %" PRIu32 ": %s, %s\n", emp->employee_number,
		       emp->family_name, emp->given_name);
	}

	emp = get_by_name("Feivush", "Georg");  /* Should check for NULL */
	printf("\nGeorg Feivush's employee number is %" PRIu32 "\n",
	       emp->employee_number);

	emp = get_by_number(4890);  /* Should check for NULL */
	printf("\nEmployee number 4890 is %s %s\n",
	       emp->given_name, emp->family_name);

	putchar('\n');

	savl_free(&employees_by_name, free_by_name);

	/* No need to free by number; nodes were embedded in employee structs */
	employees_by_number = NULL;

	return 0;
}
