// Global variables which might change based on our requests.
var movies_max = 490000;
var movies_current = 0;
var steps = 0;

var add_english = false;
var add_bw = false;
var add_early = false;
var add_silent = false;
var add_adult = false;

function uncheck_switches() {
    let switches = document.getElementsByClassName("switch");
    for (const item of switches) {
        item.checked = false;
    }
}

// Listeners
function bind_button_listeners() {
    let buttons = document.getElementsByClassName("button");
    for (const item of buttons) {
        let id = item.getAttribute("id");
        if (id === "button_start") {
            item.addEventListener("click", event => {
                event.stopPropagation();
                alert("button_start js");
            });
        } else if (id === "button_advance_false") {
            item.addEventListener("click", event => {
                event.stopPropagation();
                alert("button_advance_false js");
            });
        } else if (id === "button_advance_true") {
            item.addEventListener("click", event => {
                event.stopPropagation();
                alert("button_advance_true js");
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

// Main
uncheck_switches();
bind_switch_listeners();
bind_button_listeners();
