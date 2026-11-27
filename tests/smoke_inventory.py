"""A malformed inventory must not take down other logged-in players."""
from smoke_numbers import run
if __name__=="__main__":
    run(inventory_failures=True)
