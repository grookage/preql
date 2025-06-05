# PreQL Database Management System

PreQL is a lightweight, modular database management system written in modern C++. It provides a simple yet powerful interface for managing relational databases through SQL-like commands.

## Features

- **SQL-like Interface**: Support for basic SQL operations (CREATE, INSERT, SELECT, DELETE)
- **Buffer Management**: Efficient page management with buffer pool
- **Table Management**: Create, modify, and manage database tables
- **Interactive CLI**: User-friendly command-line interface
- **Modern C++**: Built with C++17 features and best practices

## Project Structure

```
preql/
├── include/                 # Header files
│   ├── core/               # Core database functionality
│   ├── buffer/             # Buffer management
│   ├── sql/                # SQL parsing and processing
│   └── ui/                 # User interface
├── src/                    # Source files
│   ├── core/              # Database implementation
│   ├── buffer/            # Buffer manager implementation
│   ├── sql/               # Parser implementation
│   └── ui/                # CLI implementation
├── tests/                  # Test suite
│   ├── database_test.cpp  # Database tests
│   ├── buffer_test.cpp    # Buffer manager tests
│   ├── parser_test.cpp    # Parser tests
│   └── cli_test.cpp       # CLI tests
└── CMakeLists.txt         # Build configuration
```

## Building the Project

### Prerequisites

- CMake (version 3.10 or higher)
- C++17 compatible compiler
- Google Test (for running tests)

### Build Steps

1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/preql.git
   cd preql
   ```

2. Create a build directory:
   ```bash
   mkdir build
   cd build
   ```

3. Configure and build:
   ```bash
   cmake ..
   make
   ```

4. Run tests:
   ```bash
   make test
   ```

## Usage

### Starting the Database

```bash
./preql
```

### Available Commands

#### Creating Tables

Basic table creation:
```sql
CREATE TABLE users (id INT, name VARCHAR, age INT)
```

Table with multiple columns and types:
```sql
CREATE TABLE products (
    id INT,
    name VARCHAR,
    price FLOAT,
    description VARCHAR,
    in_stock BOOLEAN
)
```

#### Inserting Data

Single row insertion:
```sql
INSERT INTO users VALUES (1, 'John Doe', 25)
```

Multiple rows (execute each separately):
```sql
INSERT INTO users VALUES (2, 'Jane Smith', 30)
INSERT INTO users VALUES (3, 'Bob Johnson', 35)
INSERT INTO users VALUES (4, 'Alice Brown', 28)
```

#### Querying Data

Basic selection:
```sql
SELECT * FROM users
```

Select specific columns:
```sql
SELECT name, age FROM users
```

With conditions:
```sql
SELECT * FROM users WHERE age > 25
```

Complex conditions:
```sql
SELECT * FROM users WHERE age > 25 AND name LIKE 'J%'
```

#### Updating Data

```sql
UPDATE users SET age = 26 WHERE name = 'John Doe'
```

#### Deleting Data

Delete specific records:
```sql
DELETE FROM users WHERE age < 30
```

Delete all records:
```sql
DELETE FROM users
```

#### Table Management

View table structure:
```sql
DESCRIBE users
```

List all tables:
```sql
SHOW TABLES
```

### Example Workflow

1. Create a new database and tables:
```sql
CREATE TABLE employees (
    id INT,
    name VARCHAR,
    department VARCHAR,
    salary FLOAT,
    hire_date DATE
)

CREATE TABLE departments (
    id INT,
    name VARCHAR,
    location VARCHAR,
    manager_id INT
)
```

2. Insert sample data:
```sql
INSERT INTO employees VALUES (1, 'John Smith', 'Engineering', 75000, '2020-01-15')
INSERT INTO employees VALUES (2, 'Sarah Johnson', 'Marketing', 65000, '2019-06-01')
INSERT INTO employees VALUES (3, 'Michael Brown', 'Engineering', 80000, '2018-03-10')

INSERT INTO departments VALUES (1, 'Engineering', 'Floor 3', 1)
INSERT INTO departments VALUES (2, 'Marketing', 'Floor 2', 2)
```

3. Query the data:
```sql
-- Find all employees in Engineering
SELECT * FROM employees WHERE department = 'Engineering'

-- Find departments and their managers
SELECT d.name, e.name as manager_name 
FROM departments d 
JOIN employees e ON d.manager_id = e.id

-- Find average salary by department
SELECT department, AVG(salary) as avg_salary 
FROM employees 
GROUP BY department
```

4. Update records:
```sql
-- Give a raise to all Engineering employees
UPDATE employees 
SET salary = salary * 1.1 
WHERE department = 'Engineering'
```

5. Delete records:
```sql
-- Remove employees who left before 2020
DELETE FROM employees 
WHERE hire_date < '2020-01-01'
```

## Architecture

### Core Components

1. **Database Core**
   - Table management
   - Data storage and retrieval
   - Transaction handling

2. **Buffer Manager**
   - Page management
   - Buffer pool implementation
   - Page replacement policies

3. **SQL Parser**
   - SQL statement parsing
   - Query validation
   - Statement processing

4. **CLI Interface**
   - Command-line interface
   - User interaction
   - Result formatting

### Design Patterns

- PIMPL (Pointer to Implementation) for implementation hiding
- RAII for resource management
- Smart pointers for memory management
- Exception handling for error management

## Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Modern C++ features and best practices
- Google Test framework for testing
- CMake build system