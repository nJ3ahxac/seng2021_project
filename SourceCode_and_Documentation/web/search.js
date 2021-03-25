// Global variables which might change based on our requests.
// Search vars.
var g_least_until_list = 1000;
var g_token = "";
var g_movies_max = 0;
var g_movies_current = 0;
var g_steps = 0;
var g_keyword = "";
var g_is_genre = false;

// Card vars.
var g_best_image = "";
var g_best_title = "";
var g_best_year = "";

// Initial vars.
var g_is_foreign = false;
var g_is_greyscale = false;
var g_is_silent = false;
var g_is_pre1980 = false;
var g_is_adult = false;

var g_prompts = [ "How do you feel about this ", 
                  "What about this ", 
                  "And this " ];

function uncheck_switches() {
    let switches = document.getElementsByClassName("switch");
    for (const item of switches) {
        item.checked = false;
    }
}

function update_progress_bar() {
    let bars = document.getElementsByClassName("progress");
    for (const item of bars) {
        let id = item.getAttribute("id");
        if (id === "progress_bar") {
            item.setAttribute("max", g_movies_max);
            item.setAttribute("value", g_movies_max - g_movies_current);
        }
    }
}

function clear_object(item) {
    while (item.firstChild && item.removeChild(item.firstChild));
}

function get_prompt() {
    let suffix = "keyword";
    if (g_is_genre === "true") {
        suffix = "genre";
    }
    return g_prompts[g_steps % g_prompts.length] + suffix + "?";
}

// Constructs a "do you want x" prompt with buttons.
function draw_column_left(parent) {
    let title = document.createElement("h1");
    title.setAttribute("class", "title has-text-centered");
    title.innerText = get_prompt();

    let subtitle = document.createElement("h2");
    subtitle.setAttribute("class", "subtitle has-text-centered");
    subtitle.innerText = "There's no rush and we don't judge";

    let word = document.createElement("h1");
    word.setAttribute("class", "title has-text-centered");
    word.setAttribute("style", "font-size: 60px;");
    word.innerHTML = "<br>" + g_keyword;

    let level = document.createElement("div");
    level.setAttribute("class", "level mt-6 pt-6");
    let include_item = document.createElement("div");
    include_item.setAttribute("class", "level-item has-text-centered");
    let include_button = document.createElement("button");
    include_button.setAttribute("class", "button is-large is-primary");
    include_button.setAttribute("id", "button_advance_include");
    include_button.innerText = "Include";
    let exclude_item = document.createElement("div");
    exclude_item.setAttribute("class", "level-item has-text-centered");
    let exclude_button = document.createElement("button");
    exclude_button.setAttribute("class", "button is-large is-primary");
    exclude_button.setAttribute("id", "button_advance_exclude");
    exclude_button.innerText = "Exclude";

    parent.appendChild(title);
    parent.appendChild(subtitle);
    parent.appendChild(word);
    parent.appendChild(level);
    level.appendChild(exclude_item);
    exclude_item.appendChild(exclude_button);
    level.appendChild(include_item);
    include_item.appendChild(include_button);
}

// Constructs a nice looking movie card.
function draw_column_right(parent) {
    let card = document.createElement("div");
    card.setAttribute("class", "card");

    let card_image = document.createElement("div");
    card_image.setAttribute("class", "card-image");

    let figure = document.createElement("figure");
    figure.setAttribute("class", "image is-3by4");
    let img = document.createElement("img");
    img.setAttribute("src", g_best_image);

    let card_content = document.createElement("div");
    card_content.setAttribute("class", "card-content");
    let content = document.createElement("div");
    content.setAttribute("class", "content");
    let title = document.createElement("div");
    title.setAttribute("class", "title");
    title.innerText = "Current best result";
    let name = document.createElement("div");
    name.setAttribute("class", "subtitle");
    name.innerText = g_best_title + ", " + g_best_year;
    /*
    let description = document.createElement("div");
    description.innerText = g_best_description;
    */

    parent.appendChild(card);
    card.appendChild(card_image);
    card_image.appendChild(figure);
    figure.appendChild(img);
    card.appendChild(card_content);
    card_content.appendChild(content);
    content.appendChild(title);
    content.appendChild(name);
}

function redraw_column_contents() {
    let columns = document.getElementsByClassName("column");
    for (const item of columns) {
        let id = item.getAttribute("id");
        if (id === "column_left") {
            clear_object(item);
            draw_column_left(item);
        } else if (id === "column_right") {
            clear_object(item);
            draw_column_right(item);
        }
    }
}

// Removes elements that exist only on the first "begin search" page.
function remove_begin_elements() {
    let begin_search_button = document.getElementById("level_buttons_begin");
    if (begin_search_button) {
        clear_object(begin_search_button);
    }
}

// Early out to avoid too esoteric movies.
async function try_list_redirect() {
    // Not initialised yet, do nothing.
    if (g_movies_current === 0) {
        return;
    }
    if (g_movies_current <= 100) {
        window.location.replace("/list");
        // Sleep, fsr javascript is still executed after replace
        await new Promise(r => setTimeout(r, 5000));
    }
}

function store_token_cookie() {
    let url = window.location.hostname;
    // We can't set secure, at least not yet, as we are not using HTTPS.
    let args = "; domain=" + url + "; path=/; samesite=lax;"
    document.cookie = "token=" + g_token + args;
}

// Regardless of the request, this function should set the correct global vars.
async function update_global_variables(json) {
    let token = json["token"];
    if (token) {
        g_token = token;
        store_token_cookie();
    }
    let max = json["max"];
    if (max) {
        g_movies_max = max;
    }
    let cur = json["cur"];
    if (cur) {
        g_movies_current = cur;
    }
    let keyword = json["keyword"];
    if (keyword) {
        g_keyword = keyword;
    }
    let is_genre = json["is_genre"];
    if (is_genre) {
        g_is_genre = is_genre;
    }
    let poster = json["poster"];
    if (poster) {
        g_best_image = poster;
    }
    let year = json["year"];
    if (year) {
        g_best_year = year;
    }
    let title = json["title"];
    if (title) {
        g_best_title = title;
    }
    let error = json["error"];
    if (error) {
        alert(error);
    }
    await try_list_redirect();
}

function set_column_padding(value) {
    let column = document.getElementById("columns_question");
    column.setAttribute("style", "padding-top: " + value + "px;");
}

function draw_token_advance() {
    update_progress_bar();
    remove_begin_elements();
    set_column_padding(33);
    redraw_column_contents();
}

// This function parses all the json and stores it in the global variables too
function redraw_box_interior() {
    // Eventually this should split into a finished page once we have exhaused
    // all questions, but for now we have only one branch.
    draw_token_advance();
    bind_button_listeners();
}

async function make_init_request() {
    let json_options = {
        "type" : "init",
        "is_foreign" : String(g_is_foreign),
        "is_greyscale" : String(g_is_greyscale),
        "is_silent" : String(g_is_silent),
        "is_pre1980" : String(g_is_pre1980),
        "is_adult" : String(g_is_adult)
    };
    let request_body = {method : "POST", body : JSON.stringify(json_options)};
    let request_url = window.location.hostname;
    await fetch(request_url, request_body)
        .then(response => response.json())
        .then(data => update_global_variables(data));
}

async function make_info_request() {
    let json_options = {
        "type" : "info",
        "token" : String(g_token),
    };
    let request_body = {method : "POST", body : JSON.stringify(json_options)};
    let request_url = window.location.hostname;
    await fetch(request_url, request_body)
        .then(response => response.json())
        .then(data => update_global_variables(data));
}

async function make_advance_request(should_remove) {
    let json_options = {
        "type" : "advance",
        "token" : String(g_token),
        "remove" : String(should_remove)
    };
    let request_body = {method : "POST", body : JSON.stringify(json_options)};
    let request_url = window.location.hostname;
    await fetch(request_url, request_body)
        .then(response => response.json())
        .then(data => update_global_variables(data));
}

async function handle_button_start(event) {
    event.stopPropagation();
    event.target.classList.add("is-loading");
    unbind_request_button_listeners();
    // These must block or we will have no data to render!
    // init also relies on info!
    await make_init_request();
    await make_info_request();
    redraw_box_interior();
}

async function handle_button_advance_include(event) {
    event.stopPropagation();
    event.target.classList.add("is-loading");
    unbind_request_button_listeners();
    ++g_steps;
    await make_advance_request(false);
    await make_info_request();
    redraw_box_interior();
}

async function handle_button_advance_exclude(event) {
    event.stopPropagation();
    event.target.classList.add("is-loading");
    unbind_request_button_listeners();
    ++g_steps;
    await make_advance_request(true);
    await make_info_request();
    redraw_box_interior();
}

// Listeners
function bind_button_listeners() {
    let buttons = document.getElementsByClassName("button");
    for (const item of buttons) {
        let id = item.getAttribute("id");
        if (id === "button_start") {
            item.addEventListener("click", handle_button_start);
        } else if (id === "button_advance_include") {
            item.addEventListener("click", handle_button_advance_include);
        } else if (id === "button_advance_exclude") {
            item.addEventListener("click", handle_button_advance_exclude);
        }
    }
}

function bind_switch_listeners() {
    let switches = document.getElementsByClassName("switch");
    for (const item of switches) {
        let id = item.getAttribute("id");
        if (id === "switch_english") {
            item.addEventListener("click", event => {
                event.stopPropagation();
                g_is_foreign = !g_is_foreign;
            });
        } else if (id === "switch_bw") {
            item.addEventListener("click", event => {
                event.stopPropagation();
                g_is_greyscale = !g_is_greyscale;
            });
        } else if (id === "switch_early") {
            item.addEventListener("click", event => {
                event.stopPropagation();
                g_is_silent = !g_is_silent;
            });
        } else if (id === "switch_silent") {
            item.addEventListener("click", event => {
                event.stopPropagation();
                g_is_pre1980 = !g_is_pre1980
            });
        } else if (id === "switch_adult") {
            item.addEventListener("click", event => {
                event.stopPropagation();
                g_is_adult = !g_is_adult;
            });
        }
    }
}

// This function unbinds buttons which make a request so that a user cannot
// press two buttons and invoke a networked race condition.
function unbind_request_button_listeners() {
    let buttons = document.getElementsByClassName("button");
    for (const item of buttons) {
        let id = item.getAttribute("id");
        if (id === "button_start") {
            item.removeEventListener("click", handle_button_start);
        } else if (id === "button_advance_include") {
            item.removeEventListener("click", handle_button_advance_include);
        } else if (id === "button_advance_exclude") {
            item.removeEventListener("click", handle_button_advance_exclude);
        }
    }
}

// Called once.
uncheck_switches();
bind_switch_listeners();
bind_button_listeners();
