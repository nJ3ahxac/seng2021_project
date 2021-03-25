var g_token = "";
// List arguments.
var g_begin = 0;
var g_count = 100;
// List return json.
var g_results = {};

async function unshow_pageloader() { 
    let pageloader = document.getElementById("loader");
    pageloader.classList.remove("is-active"); 
}

async function redirect_search() {
    window.location.replace("/search");
    await new Promise(r => setTimeout(r, 5000));
}

async function update_global_variables(json) {
    let error = json["error"];
    if (error) {
        // Any errors will redirect to search.
        await redirect_search();
    }
    g_results = json;
}

function read_token_cookie() {
    let token_cookie_value = document.cookie.split("; ")
                           .find(row => row.startsWith("token="))
                           .split('=')[1];
    if (!token_cookie_value) {
        return false;
    }
    g_token = token_cookie_value;
    return true;
}

async function make_results_request() {
    let json_options = {
        "type" : "results",
        "token" : String(g_token),
        "begin" : String(g_begin),
        "count" : String(g_count)
    };
    let request_body = {method : "POST", body : JSON.stringify(json_options)};
    let request_url = window.location.hostname;
    await fetch(request_url, request_body)
        .then(response => response.json())
        .then(data => update_global_variables(data));
}

function draw_results() {
}

async function handle_page_load() {
    if (!read_token_cookie()) {
        // No token? You have to go back.
        await redirect_search();
    }
    await make_results_request();
    draw_results();
    unshow_pageloader();
}

handle_page_load();
