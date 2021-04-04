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

// Returns true if it exists.
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

function populate_top_carousel() {
    let rmovies = g_results.movies.sort(function(a, b) {
        return parseFloat(a.rating) < parseFloat(b.rating);
    });
    let parent = document.getElementById("top_carousel");
    for (let i = 0; i < rmovies.length; ++i) {
        let item = document.createElement("div");
        item.setAttribute("class", "item-" + i);
        let card = document.createElement("div");
        card.setAttribute("class", "card");
        card.setAttribute("movie", i);

        let card_image = document.createElement("div");
        card_image.setAttribute("class", "card-image");

        let figure = document.createElement("figure");
        figure.setAttribute("class", "image is-3by4");
        let img = document.createElement("img");
        img.setAttribute("src", rmovies[i]["poster"]);

        let card_content = document.createElement("div");
        card_content.setAttribute("class", "card-content");
        card_content.setAttribute("style", "min-height: 117px;");
        let columns = document.createElement("div");
        columns.setAttribute("class", "columns is-vcentered is-centered");
        let column = document.createElement("div");
        column.setAttribute("class", "column");
        let content = document.createElement("div");
        content.setAttribute("class", "content");
        let title = document.createElement("div");
        title.setAttribute("class", "title is-5");
        title.setAttribute("style",
                           "min-width: 0; overflow: hidden;" +
                               "text-overflow: ellipsis; max-height: 45px");
        title.innerText = rmovies[i]["title"];
        let info = document.createElement("div");
        info.setAttribute("class", "subtitle is-6");
        info.innerText =
            rmovies[i]["year"] + ", " + rmovies[i]["rating"] + "/10";

        parent.appendChild(item);
        item.appendChild(card);
        card.appendChild(card_image);
        card_image.appendChild(figure);
        figure.appendChild(img);
        card.appendChild(card_content);
        card_content.appendChild(columns);
        columns.appendChild(column);
        column.appendChild(content);
        content.appendChild(title);
        content.appendChild(info);
    }
}

function attach_carousels() {
    bulmaCarousel.attach("#top_carousel", {
        navigation : true,
        pagination : false,
        infinite : true,
        slidesToShow : 2,
        slidesToScroll: 2
    });
}

function update_cell(cell, rpos, cpos) {
    let contents = "";
    let value = null;
    switch (cpos) {
        case 0: {
            contents = g_results.movies[rpos]["title"];
            break;
        }
        case 1: {
            contents = g_results.movies[rpos]["year"];
            break;
        }
        case 2: {
            contents = g_results.movies[rpos]["rating"];
            break;
        }
        case 3: {
            contents = g_results.movies[rpos]["votes"];
            break;
        }
        case 4: {
            contents = g_results.movies[rpos]["runtime"];
            if (contents === "N/A") {
                value = "0";
            } else {
                value = contents.slice(0, -4);
            }
            break;
        }
    }
    cell.innerHTML = contents;
    if (value) {
        cell.setAttribute("value", value);
    }
}

function populate_table() {
    let table = document.getElementById("movie_table");
    let table_body = table.tBodies[0];
    table_body.innerHTML = ""; // resets the table body, if it exists.
    for (let i = 0; i < g_results.movies.length; ++i) {
        let row = table_body.insertRow(-1);
        for (let j = 0; j < 5; ++j) {
            let cell = row.insertCell(j);
            update_cell(cell, i, j);
        }
        row.setAttribute("movie", i); // store movie for click event
    }
}

function attach_table() {
    const getCellValue = (tr, idx) =>
        tr.children[idx].getAttribute("value") || tr.children[idx].textContent;

    const comparer = (idx, asc) => (a, b) =>
        ((v1, v2) => v1 !== '' && v2 !== '' && !isNaN(v1) && !isNaN(v2)
                         ? v1 - v2
                         : v1.toString().localeCompare(v2))(
            getCellValue(asc ? a : b, idx), getCellValue(asc ? b : a, idx));

    // Set sorting of table events
    document.querySelectorAll('th').forEach(
        th => th.addEventListener('click', function() {
            document.querySelectorAll('th').forEach(th => {
                if (th.lastElementChild) {
                    th.removeChild(th.lastElementChild);
                }
            });
            let table = document.getElementById("movie_table");
            const table_body = table.tBodies[0];
            Array.from(table_body.querySelectorAll('tr'))
                .sort(comparer(Array.from(th.parentNode.children).indexOf(th),
                               this.asc = !this.asc))
                .forEach(tr => table_body.appendChild(tr));
            let sorted = document.createElement("i");
            sorted.className = "fas fa-sort";
            th.appendChild(sorted);
        }))
}

function draw_results() {
    populate_top_carousel();
    attach_carousels();
    populate_table();
    attach_table();
}

// Since we can't use the youtube API for this, we will scrape our results.
// This does not work because of CORS. We could forward this to our server,
// but the added complexity just for a youtube video is bad. We will defer
// running trailers until we can find a better solution.
async function get_yt_code(string) {
    // Not many urls like spacebars.
    let search_string = string.split(' ').join('+');
    let url = "https://www.youtube.com/results?search_query=" + search_string;
    console.log("Making request to: " + url);
    let results = "";
    await fetch(url)
        .then(response => response.text())
        .then(response => { results = response; });
    console.log(results);
    let scrape_start = "watch?v=";
    let scrape_end = "\"";
    let startpos = results.indexOf("watch?v=");
    let endpos = results.indexOf(scrape_end, startpos);
    return results.substring(startpos, endpos);
}

async function draw_movie_modal(entry) {
    let modal = document.createElement("div");
    modal.setAttribute("class", "modal is-active");
    let modal_background = document.createElement("div");
    modal_background.setAttribute("class", "modal-background");
    let modal_close = document.createElement("button");
    modal_close.setAttribute("class", "modal-close is-large");

    let modal_content = document.createElement("div");
    modal_content.setAttribute("class", "modal-content");
    let box = document.createElement("div");
    box.setAttribute("class", "box");
    let columns = document.createElement("div");
    columns.setAttribute("class", "columns is-vcentered is-mobile");

    let left_column = document.createElement("div");
    left_column.setAttribute("class", "column is-two-fifths");
    let card = document.createElement("div");
    card.setAttribute("class", "card");
    let card_image = document.createElement("div");
    card_image.setAttribute("class", "card-image");
    let figure = document.createElement("figure");
    figure.setAttribute("class", "image is-3by4");
    let img = document.createElement("img");
    img.setAttribute("src", g_results.movies[entry]["poster"]);

    let right_column = document.createElement("div");
    right_column.setAttribute("class", "column mr-4");
    let right_columns = document.createElement("div");
    right_columns.setAttribute("class", "columns is-multiline is-mobile");

    let right_first_column = document.createElement("div");
    right_first_column.setAttribute("class", "column is-full");
    let right_second_column = document.createElement("div");
    right_second_column.setAttribute("class", "column is-half");
    let right_third_column = document.createElement("div");
    right_third_column.setAttribute("class", "column is-half");
    let right_fourth_column = document.createElement("div");
    right_fourth_column.setAttribute("class", "column is-full");
    let right_fifth_column = document.createElement("div");
    right_fifth_column.setAttribute("class", "column is-full");
    let right_sixth_column = document.createElement("div");
    right_sixth_column.setAttribute("class", "column is-half");
    let right_seventh_column = document.createElement("div");
    right_seventh_column.setAttribute("class", "column is-half");

    let title = document.createElement("div");
    title.setAttribute("class", "title has-text-centered");
    title.innerText = g_results.movies[entry]["title"];
    let year = document.createElement("div");
    year.setAttribute("class", "subtitle has-text-centered");
    year.innerHTML =
        "<strong>Year: </strong>" + g_results.movies[entry]["year"];
    let rating = document.createElement("div");
    rating.setAttribute("class", "subtitle has-text-centered");
    rating.innerHTML =
        "<strong>Rating: </strong>" + g_results.movies[entry]["rating"];
    let popularity = document.createElement("div");
    popularity.setAttribute("class", "subtitle has-text-centered");
    popularity.innerHTML =
        "<strong>Popularity: </strong>" + g_results.movies[entry]["votes"];
    let runtime = document.createElement("div");
    runtime.setAttribute("class", "subtitle has-text-centered");
    let runtime_value = g_results.movies[entry]["runtime"];
    if (runtime_value != "N/A") {
        runtime_value = runtime_value.slice(0, -2);
    }
    runtime.innerHTML = "<strong>Runtime: </strong>" + runtime_value;
    let director = document.createElement("div");
    director.setAttribute("class", "subtitle has-text-centered");
    director.innerHTML = "<strong>Directed by: </strong>" +
                         g_results.movies[entry]["director"].split(',')[0];
    let box_office = document.createElement("div");
    box_office.setAttribute("class", "subtitle has-text-centered");
    let dollar_value = g_results.movies[entry]["box_office"];
    if (dollar_value === "N/A") {
        dollar_value = "Unknown";
    }
    box_office.innerHTML = "<strong>Box Office: </strong>" + dollar_value;

    let plot = document.createElement("div");
    plot.setAttribute("class", "content is-medium");
    plot.innerHTML = "<strong>Plot: </strong>" +
                     g_results.movies[entry].plot;

    document.body.append(modal);
    modal.appendChild(modal_background);
    modal.appendChild(modal_close);
    modal.appendChild(modal_content);
    modal_content.appendChild(box);
    box.appendChild(columns);

    columns.appendChild(left_column);
    left_column.appendChild(card);
    card.appendChild(card_image);
    card_image.appendChild(figure);
    figure.appendChild(img);

    columns.appendChild(right_column);
    right_column.appendChild(right_columns);
    right_columns.appendChild(right_first_column);
    right_first_column.appendChild(title);
    right_columns.appendChild(right_second_column);
    right_second_column.appendChild(year);
    right_columns.appendChild(right_third_column);
    right_third_column.appendChild(runtime);

    right_columns.appendChild(right_fourth_column);
    right_fourth_column.appendChild(director);
    right_columns.appendChild(right_fifth_column);
    right_fifth_column.appendChild(box_office);

    right_columns.appendChild(right_sixth_column);
    right_sixth_column.appendChild(rating);
    right_columns.appendChild(right_seventh_column);
    right_seventh_column.appendChild(popularity);

    box.appendChild(plot);

    modal_background.addEventListener("click", event => {
        event.stopPropagation();
        modal.remove();
    });
    modal_close.addEventListener("click", event => {
        event.stopPropagation();
        modal.remove();
    });
}

// For every div with a "movie=n" (where n maps to g_results.movies[n]), this
// function binds a modal click listener with that info.
function bind_movie_listeners() {
    let divs = document.getElementsByTagName("*");
    for (const div of divs) {
        let n = parseInt(div.getAttribute("movie"));
        if (isNaN(n)) {
            continue;
        }
        div.classList.add("is-clickable");
        div.addEventListener("click", event => {
            event.stopPropagation();
            draw_movie_modal(n);
        });
    }
}

function bind_button_listeners() {
    let button_save = document.getElementById("button_save");
    button_save.addEventListener("click", event => {
        let contents = JSON.stringify(g_results);
        // TODO
        event.stopPropagation();
    });
    let button_print = document.getElementById("button_print");
    button_print.addEventListener("click", event => {
        window.print();
        event.stopPropagation();
    });
}

function bind_listeners() {
    bind_movie_listeners();
    bind_button_listeners();
}

function fix_visual_bugs() {
    let box_shader_fix = document.getElementsByClassName("slider-container")[0];
    let style = box_shader_fix.getAttribute("style");
    box_shader_fix.setAttribute("style", style + "height: 490px");
}

async function handle_page_load() {
    if (!read_token_cookie()) {
        await redirect_search();
    }
    await make_results_request();
    draw_results();
    bind_listeners();
    fix_visual_bugs();
    await new Promise(r => setTimeout(r, 1000)); // fake loading screen
    unshow_pageloader();
}

handle_page_load();
