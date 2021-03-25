var g_token = "";

async function unshow_pageloader() { 
    let pageloader = document.getElementById("loader");
    pageloader.classList.remove("is-active"); 
}

function update_global_variables() {
}

function read_token_cookie() {
    let token_cookie_value = document.cookie.split("; ")
                           .find(row => row.startsWith("token="))
                           .split('=')[1];
    if (token_cookie_value) {
        g_token = token_cookie_value;
    }
}

function handle_page_load() {
    read_token_cookie();
    if (g_token != "") {
        alert("Read token cookie as: " + g_token);
    } else {
        alert("Failed to read token cookie. Does it exist?");
    }
    unshow_pageloader();
}

handle_page_load();
