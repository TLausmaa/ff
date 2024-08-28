use std::fs::DirEntry;
use tokio;
use tokio::task::JoinHandle;

struct Args {
    dir: String,
    name: String,
    ignored_extensions: Vec<String>,
    only_extensions: Vec<String>,
}

fn get_dir_entries(dir: String) -> Vec<DirEntry> {
    let entries = std::fs::read_dir(dir).unwrap();
    let mut results = Vec::new();
    for entry in entries {
        let entry = entry.unwrap();
        results.push(entry);
    }
    return results
}

fn pick_args_until_next_option(args: &Vec<String>, pos: usize) -> Vec<String> {
    let mut result: Vec<String> = Vec::new();
    let mut i = pos + 1;
    while i < args.len() {
        if args[i].starts_with("-") {
            break;
        }
        result.push(args[i].clone());
        i += 1;
    }
    return result;
}

fn parse_option(args: &Vec<String>, pos: usize, arguments: &mut Args) {
    if args[pos] == "-i" {
        arguments.ignored_extensions = pick_args_until_next_option(args, pos);
    } else if args[pos] == "-e" {
        arguments.only_extensions = pick_args_until_next_option(args, pos);
    }
}

fn get_args() -> Args {
    let mut arguments: Args = Args {
        dir: String::from(""),
        name: String::from(""),
        ignored_extensions: vec![],
        only_extensions: vec![],
    };

    let args: Vec<String> = std::env::args().collect();
    if args.len() < 2 {
        println!("Usage:\n\t{} <name>\n\t{} <dir> <name>", args[0], args[0]);
        std::process::exit(1);
    }
    
    let cwd = std::env::current_dir().unwrap().display().to_string();

    for i in 1..args.len() {
        if i == 1 {
            arguments.name = args[i].clone();
        } else if i == 2 {
            if args[i].starts_with("-") {
                parse_option(&args, i, &mut arguments);
            } else {
                arguments.dir = args[i-1].clone();
                arguments.name = args[i].clone();
            }
        } else {
            if args[i].starts_with("-") {
                parse_option(&args, i, &mut arguments);
            }
        }
    }

    if arguments.dir == "" {
        arguments.dir = cwd;
    }

    if !std::path::Path::new(&arguments.dir).is_dir() {
        println!("{} is not a directory", arguments.dir);
        std::process::exit(1);
    }

    return arguments;
}

fn is_allow_listed_extension(entry: DirEntry, file_name: &String, allowed_extensions: &Vec<String>) -> bool {
    if entry.path().is_file() && !allowed_extensions.is_empty() && file_name.contains(".") {
        let this_file_extension = file_name.split(".").last().unwrap();
        for allowed_ext in allowed_extensions {
            if allowed_ext == this_file_extension {
                return true;
            }
        }
    }
    return false;
}

fn is_ignored_file(entry: DirEntry, file_name: &String, ignored_extensions: &Vec<String>) -> bool {
    if entry.path().is_file() && !ignored_extensions.is_empty() && file_name.contains(".") {
        let this_file_extension = file_name.split(".").last().unwrap();
        for ignored_ext in ignored_extensions {
            if ignored_ext == this_file_extension {
                return true;
            }
        }
    }
    return false;
}

fn is_ignored_dir(entry: &DirEntry, file_name: &String) -> bool {
    if entry.path().is_dir() {
        // Default hard-coded directories we ignore. Maybe we should ignore these if -i or -e are specified? 
        return file_name == "node_modules" || file_name == ".git" || file_name == ".vscode" || file_name == ".yarn";        
    }
    return false;
}

#[tokio::main]
async fn main() {
    let args = get_args();
    let mut next_dirs: Vec<String> = Vec::new();
    next_dirs.push(args.dir);

    while !next_dirs.is_empty() {
        let mut handles: Vec<JoinHandle<Vec<DirEntry>>> = Vec::new();
        for d in &next_dirs {
            let d = d.clone();
            let handle = tokio::spawn(async move {
                let entries = get_dir_entries(d);
                return entries;
            });
            handles.push(handle);
        }
        
        next_dirs.clear();

        for handle in handles {
            let result = handle.await;
            match result {
                Ok(res) => {
                    for entry in res {
                        let path = entry.path();
                        let name = path.display().to_string();
                        let file_name = path.file_name().unwrap().to_string_lossy().to_string();

                        if is_ignored_dir(&entry, &file_name) {
                            continue;
                        }

                        if args.only_extensions.len() > 0 && entry.path().is_file() {
                            if !is_allow_listed_extension(entry, &file_name, &args.only_extensions) {
                                continue;
                            }
                        } else {
                            if is_ignored_file(entry, &file_name, &args.ignored_extensions) {
                                continue;
                            }
                        }

                        if path.is_dir() {
                            next_dirs.push(name);
                        } else {
                            if file_name.contains(&args.name) {
                                println!("{}", name);
                            }
                        }
                    }
                },
                Err(e) => println!("Task failed with error: {:?}", e),
            }
        }
    }
}
