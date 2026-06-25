# Your Core Principles

All code you write MUST be fully optimized.

"Fully optimized" includes:

- maximizing algorithmic big-O efficiency for memory and runtime
- using parallelization and vectorization where appropriate
- following proper style conventions for the code language (e.g. maximizing code reuse (DRY))
- no extra code beyond what is absolutely necessary to solve the problem the user provides (i.e. no technical debt)
- **ALWAYS** be explicit, concise, and sacrifice grammar for the sake of concision in all interactions and communications with the user

If the code is not fully optimized before handing off to the user, you will be fined $100. You have permission to do another pass of the code if you believe it is not fully optimized.

## Data Management and Storage

- Decouple data storage from business logic
- Use environment variables for configuration (e.g., database URLs, API keys)
- **ALWAYS** Create a `DataManager` class to handle all interactions with data storage (e.g., databases, file systems)
- Ensure the `DataManager` class has methods for CRUD operations (Create, Read, Update, Delete) for each type of data storage used
- `DataManager` can be extended to support multiple storage backends (e.g., SQL databases, NoSQL databases, file systems) as needed
- `DataManager` can be configurable to allow for different storage backends to be used interchangeably without affecting the business logic of the application

## Security

- **NEVER** store secrets, API keys, or passwords in code. Only store them in `.env`.
  - Ensure `.env` is declared in `.gitignore`.
  - **NEVER** print or log URLs to console if they contain an API key.
- **MUST** use environment variables for sensitive configuration
- **NEVER** log sensitive information (passwords, tokens, PII)