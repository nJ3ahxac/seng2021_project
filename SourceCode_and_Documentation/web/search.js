// Global variables which might change based on our requests.
var movies_max = 200000;
var movies_current = 0;
var steps = 0;
var keyword = "action";

var best_image = "https://m.media-amazon.com/images/M/MV5BZjdkOTU3MDktN2IxOS00OGEyLWFmMjktY2FiMmZkNWIyODZiXkEyXkFqcGdeQXVyMTMxODk2OTU@._V1_.jpg";
var best_title = "Interstellar";
var best_description = "In the future...";
var best_rating = "98";
var best_year = "2014";

var add_english = false;
var add_bw = false;
var add_early = false;
var add_silent = false;
var add_adult = false;

var questions = ["How do you feel about this genre?"]

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
            item.setAttribute("max", movies_max); // not necessary after 1st
            item.setAttribute("value", movies_current);
        }
    }
}

function clear_object(item) {
    while (item.firstChild && item.removeChild(item.firstChild));
}

// Constructs a "do you want x" prompt with buttons.
function draw_column_left(parent) {
    let title = document.createElement("h1");
    title.setAttribute("class", "title has-text-centered");
    title.innerText = questions[steps];

    let subtitle = document.createElement("h2");
    subtitle.setAttribute("class", "subtitle has-text-centered");
    subtitle.innerText = "There's no rush and we don't judge";

    let word = document.createElement("h1");
    word.setAttribute("class", "title has-text-centered");
    word.setAttribute("style", "font-size: 60px;");
    word.innerHTML = "<br>" + keyword;

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
    img.setAttribute("src", best_image);

    let card_content = document.createElement("div");
    card_content.setAttribute("class", "card-content");
    let content = document.createElement("div");
    content.setAttribute("class", "content");
    let title = document.createElement("div");
    title.setAttribute("class", "title");
    title.innerText = "Current best result";
    let name = document.createElement("div");
    name.setAttribute("class", "subtitle");
    name.innerText = best_title + ", " + best_year;
    let description = document.createElement("div");
    description.innerText = best_description;

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
    // remove the contents of the column
    // !! THIS IS NOT COLUMNS, IT IS COLUMN DIV OBJECT
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

function redraw_buttons() {
    let levels = document.getElementsByClassName("level");
    for (const item of levels) {
        let id = item.getAttribute("id");
        if (id === "level_button") {
            clear_object(item);
            let level_one = document.createElement("div");
            level_one.setAttribute("class", "level-item has-text-centered");
            let accept_button = document.createElement("button");
            accept_button.setAttribute("class", "button is-large is-primary");
            accept_button.innerText = "Yes please";

            let level_two = document.createElement("div");
            level_two.setAttribute("class", "level-item has-text-centered");
            let deny_button = document.createElement("button");
            deny_button.setAttribute("class", "button is-large is-warning");
            deny_button.innerText = "No thanks";

            item.appendChild(level_one);
            level_one.appendChild(accept_button);
            item.appendChild(level_two);
            level_one.appendChild(deny_button);
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

function update_global_variables(json) {
    movies_current = 10500;
    // It's a bit pointless to write this if we haven't written the server
    // post response yet.
}

// This function parses all the json and stores it in the global variables too
function redraw_box_interior(json) {
    /*
    if (json["advanced"] === "false") {
        // SHOW END PAGE
        return;
    }
    */

    update_global_variables(json);
    update_progress_bar();

    remove_begin_elements();
    redraw_column_contents();

    // New buttons require rebinding.
    bind_button_listeners();
}

// Listeners
function bind_button_listeners() {
    let buttons = document.getElementsByClassName("button");
    for (const item of buttons) {
        let id = item.getAttribute("id");
        if (id === "button_start") {
            item.addEventListener("click", event => {
                event.stopPropagation();
                redraw_box_interior("asdf");
            });
        } else if (id === "button_advance_include") {
            item.addEventListener("click", event => {
                event.stopPropagation();
                alert("button_advance_include js");
            });
        } else if (id === "button_advance_exclude") {
            item.addEventListener("click", event => {
                event.stopPropagation();
                alert("button_advance_exclude js");
            });
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
                add_english = !add_english;
            });
        } else if (id === "switch_bw") {
            item.addEventListener("click", event => {
                event.stopPropagation();
                add_bw = !add_bw;
            });
        } else if (id === "switch_early") {
            item.addEventListener("click", event => {
                event.stopPropagation();
                add_early = !add_early;
            });
        } else if (id === "switch_silent") {
            item.addEventListener("click", event => {
                event.stopPropagation();
                add_silent = !add_silent;
            });
        } else if (id === "switch_adult") {
            item.addEventListener("click", event => {
                event.stopPropagation();
                add_adult = !add_adult;
            });
        }
    }
}

// Called once.
uncheck_switches();
bind_switch_listeners();
bind_button_listeners();
