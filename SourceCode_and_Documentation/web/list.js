// List javascript. This should read the token cookie and fill the tables based on it. For now, it's just showing off a bulma loading screen.
async function unshow_pageloader() {
    await new Promise(r => setTimeout(r, 2000));
    let pageloader = document.getElementById("loader");
    pageloader.classList.remove("is-active");
}

unshow_pageloader();
